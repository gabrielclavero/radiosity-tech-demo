//------------------------------------------------------------------------------------------
// File: Scene.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Scene.h"

namespace DTFramework
{

const float Scene::Z_FAR = 1000.0f;
const float Scene::Z_NEAR = 0.1f;

const UINT Scene::SHADOW_MAP_SIZE = 800;

const float Scene::LIGHT_VOLUME_WIDTH = 500.0f;
const float Scene::LIGHT_VOLUME_HEIGHT = 500.0f;
const float Scene::LIGHT_Z_NEAR = 1.0f;
const float Scene::LIGHT_Z_FAR = 1000.0f;

const float Scene::TRANSPARENCY_BOUNDARY = 0.15f;

Scene::Scene(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_commonShader(d3d), m_sceneMesh(0), m_zFar(Z_FAR), m_zNear(Z_NEAR), 
m_shadowMapsSize(SHADOW_MAP_SIZE), m_scale(1.0f), m_showSky(1), m_ready(false)
{

}

Scene::~Scene()
{
	SAFE_DELETE(m_sceneMesh);
}

HRESULT Scene::Init(const wstring &sceneFile, Camera * const camera, Light * const light)
{
	_ASSERT(!m_ready);

	_ASSERT(camera && light);

	if(m_ready || (!camera || !light)) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Scene::Init");
		return E_FAIL;
	}

	HRESULT hr;

	wstring file = SCENES_DIRECTORY + sceneFile;

	if(FAILED(hr = LoadSceneFromFile(file, camera, light))) return hr;

	//camera projection matrix
	float aspect = m_d3dManager.GetViewPort().Width / ( FLOAT ) m_d3dManager.GetViewPort().Height;
	float fov =  static_cast<float> (D3DX_PI) * 0.25f;
	camera->SetProjectionMatrix(aspect, fov, m_zNear, m_zFar);

	if(m_sceneMeshProperties.file.length() == 0) {
		MessageBox(NULL, L"Archivo de escena no contiene un objeto 3D.", L"Error", MB_OK);
		return E_FAIL;
	}

	//para el único objeto .obj especificado en el archivo de la escena vamos a crear el Mesh asociado
	if((m_sceneMesh = new (std::nothrow) Mesh(m_d3dManager)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED(hr = m_sceneMesh->Init(m_sceneMeshProperties.file, m_sceneMeshProperties.num_vertices, 
	                                 m_sceneMeshProperties.num_normals, m_sceneMeshProperties.num_texCoords))) 
	{										
		return hr;
	}

	//material shaders
	if(FAILED(hr = m_commonShader.Init() )) return hr;

	m_ready = true;

	return hr;
}


//------------------------------------------------------------------------------------------
// activeLights == 0 => no utilizaremos luces direccionales en esta renderización. Por lo tanto, no utilizaremos el mapa de sombras.
//
// light == NULL => no actualizaremos la luz en esta renderización PERO si activeLights > 0 sí habrá luz en la misma.
//
// GIData == NULL => no utilizaremos iluminación global en esta renderización.
//------------------------------------------------------------------------------------------
HRESULT Scene::Render(const D3DXVECTOR3 * const cameraPos, const LightProperties * const light, const UINT activeLights, ID3D11ShaderResourceView *shadowMap,
                      const D3DXMATRIX * const lightVPM, ID3D11ShaderResourceView *GIData, const D3DXMATRIX * const viewProjection)
{
	_ASSERT(m_ready);

	if(!m_ready || !cameraPos || !viewProjection) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Scene::Render");
		return E_FAIL;
	}

	HRESULT hr;

	//variables del frame
	if(FAILED( hr = m_commonShader.SetShaderVariablesPerFrame(*cameraPos, light, shadowMap, activeLights ) ) ) return hr;
	
	//variables del objeto (mesh) actual
	if(FAILED(hr = m_commonShader.SetShaderVariablesPerObject((*viewProjection), lightVPM, GIData) ) ) return hr;

	//itero por todos los subsets de la scene mesh
	const UINT nAttributes = m_sceneMesh->GetAttributeTableEntries();
	for(UINT iSubset = 0; iSubset < nAttributes; ++iSubset ) 
	{
		//tomar el subset material
		const Material * const material = m_sceneMesh->GetSubsetMaterial(iSubset);

		if(!material) return E_FAIL;

		if(FAILED(hr = m_commonShader.SetShaderVariablesPerMaterial(*material) )) return hr;

		if(FAILED(hr = m_commonShader.SetTechnique(*material, GIData ? true : false) )) return hr;

		if(FAILED( hr = m_d3dManager.ApplyEffectPass( m_commonShader.GetTechnique()->GetPassByIndex(0), 0 ) )) return hr;
			
		if(FAILED(hr = m_sceneMesh->Render(iSubset))) return hr;
	}

	return S_OK;
}

//dibuja la scene mesh sin setear pipeline states ni techniques ni render targets. Se toma el estado que esté configurado actualmente
HRESULT Scene::DrawSceneMesh() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Scene::DrawSceneMesh");
		return E_FAIL;
	}

	HRESULT hr;

	const UINT nAttributes = m_sceneMesh->GetAttributeTableEntries();
	for(UINT iSubset = 0; iSubset < nAttributes; ++iSubset ) 
	{
		const Material * const material = m_sceneMesh->GetSubsetMaterial( iSubset );

		if(!material) return E_FAIL;

		if(material->GetAlpha() < TRANSPARENCY_BOUNDARY) continue;		//materiales transparentes dejan pasar la luz

		if(FAILED ( hr = m_sceneMesh->Render(iSubset) ) ) return hr;
	}

	return S_OK;
}

HRESULT Scene::LoadSceneFromFile(const wstring &sceneFile, Camera * const camera, Light * const light)
{
	_ASSERT(camera && light);

	HRESULT hr;

	static const int BUFFERSIZE = 4096;
	static const int MAX_LINE_LENGTH = 100;

	char readBuffer[BUFFERSIZE] = {0};
	HANDLE hFile;
	DWORD bytesRead;

	WCHAR *wstrNameC = NULL;

	//file input
	if( (hFile = CreateFile(sceneFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE ) {
		ErrorWarning(L"Scene::LoadSceneFromFile-->CreateFile");
		return E_FAIL;
	}

	try 
	{
		if(!ReadFile(hFile, readBuffer, BUFFERSIZE-1, &bytesRead, NULL)) {
			ErrorWarning(L"Scene::LoadSceneFromFile-->ReadFile");
			throw 'e';
		}

		//pasamos el buffer leído a un stringstream para una fácil extracción de datos con formato
		stringstream inputFile(stringstream::in | stringstream::out);
		inputFile.exceptions( std::stringstream::failbit | std::stringstream::badbit );

		inputFile << readBuffer;

		//parseo del archivo .txt de la escena
		string strCommand;
		bool isLightActive = false, is3DObjectActive = false, isCameraActive = false;
	
		LightProperties lightProperties;
		LightType lightType;
		float z_far=LIGHT_Z_FAR, z_near=LIGHT_Z_NEAR;
		float lightHeight = LIGHT_VOLUME_HEIGHT, lightWidth = LIGHT_VOLUME_WIDTH;		//solo para luces direccionales
		UINT objectsCount = 0;

		while( !inputFile.eof() && inputFile.peek() != EOF ) 
		{
			while(isspace(inputFile.peek()) && !inputFile.eof()) inputFile.ignore();
			if(inputFile.eof()) break;

			inputFile >> strCommand;

			if(strCommand == "#") 
			{
				//comentario
			} 
			else if(strCommand == "newlight") 
			{
				if(is3DObjectActive || isCameraActive || isLightActive)
					throw SCENE_FILE_ERROR;
				
				ZeroMemory(&lightProperties, sizeof(LightProperties));

				lightProperties.on = 1;			//inicialmente la luz estará encendida

				isLightActive = true;
			}
			else if(strCommand == "endlight")
			{
				if(!isLightActive) throw SCENE_FILE_ERROR;
				if(z_near == z_far) throw SCENE_FILE_ERROR;
				if(lightWidth <= 0 || lightHeight <= 0) throw SCENE_FILE_ERROR;
				isLightActive = false;
				light->SetProperties(lightProperties, lightType);
				light->SetZNear(z_near);
				light->SetZFar(z_far);
				light->SetDirectionalLightVolume(lightWidth, lightHeight);
				z_far = LIGHT_Z_FAR;
				z_near = LIGHT_Z_NEAR;
				lightHeight = LIGHT_VOLUME_HEIGHT;
				lightWidth = LIGHT_VOLUME_WIDTH;
			}
			else if(strCommand == "type")
			{
				inputFile >> strCommand;
				if(strCommand == "POINT")
					lightType = POINT_LIGHT;
				else if(strCommand == "DIRECTIONAL")
					lightType = DIRECTIONAL_LIGHT;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "position")
			{
				float x, y, z;
				inputFile >> x >> y >> z;
				D3DXVECTOR3 tmp(x,y,z);
				if(isLightActive)
					lightProperties.pos = tmp;
				else if(is3DObjectActive)
					m_sceneMeshProperties.pos = tmp;
				else if(isCameraActive)
					camera->SetEyeVector(tmp);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "direction")
			{
				float x, y, z;
				inputFile >> x >> y >> z;

				D3DXVECTOR3 tmp(x,y,z);
				if(isLightActive)
					lightProperties.dir = tmp;
				else if(isCameraActive)
					camera->SetAtVector(tmp);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "ambient")
			{
				float r,g,b,a;
				inputFile >> r >> g >> b >> a;

				if(isLightActive)
					lightProperties.ambient = D3DXCOLOR(r,g,b,a);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "diffuse")
			{
				float r,g,b,a;
				inputFile >> r >> g >> b >> a;

				if(isLightActive)
					lightProperties.diffuse = D3DXCOLOR(r,g,b,a);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "specular")
			{
				float r,g,b,a;
				inputFile >> r >> g >> b >> a;

				if(isLightActive)
					lightProperties.specular = D3DXCOLOR(r,g,b,a);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "attenuation")
			{
				float x,y,z;
				inputFile >> x >> y >> z;

				if(x <= 0 && y <= 0 && z <= 0) throw SCENE_FILE_ERROR;

				if(isLightActive)
					lightProperties.att = D3DXVECTOR3(x,y,z);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "range")
			{
				if(isLightActive)
					inputFile >> lightProperties.range;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "shadowmapbias")
			{
				if(isLightActive)
					inputFile >> lightProperties.shadowMapBias;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "znear")
			{
				if(isLightActive)
					inputFile >> z_near;
				else 
					throw SCENE_FILE_ERROR;
			} 
			else if(strCommand == "zfar")
			{
				if(isLightActive)
					inputFile >> z_far;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "lightvolumeheight")
			{
				if(isLightActive)
					inputFile >> lightHeight;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "lightvolumewidth")
			{
				if(isLightActive)
					inputFile >> lightWidth;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "newcamera")
			{
				if(is3DObjectActive || isCameraActive || isLightActive) 
					throw SCENE_FILE_ERROR;
				
				isCameraActive = true;
			}
			else if(strCommand == "endcamera")
			{
				if(!isCameraActive) 
					throw SCENE_FILE_ERROR;

				isCameraActive = false;

				if(m_zNear == m_zFar) throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "up")
			{
				float x,y,z;
				inputFile >> x >> y >> z;
				D3DXVECTOR3 tmp(x,y,z);

				if(isCameraActive)
					camera->SetUpVector(tmp);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "shadowmapsize")
			{
				inputFile >> m_shadowMapsSize;
				if(m_shadowMapsSize <= 0) throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "projzfar")
			{
				if(isCameraActive)
					inputFile >> m_zFar;
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "projznear")
			{
				if(isCameraActive)
					inputFile >> m_zNear;
				else
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "newobject")
			{
				if(is3DObjectActive || isCameraActive || isLightActive || objectsCount > 0) 
					throw SCENE_FILE_ERROR;

				is3DObjectActive = true;
				++objectsCount;
			}
			else if(strCommand == "endobject")
			{
				if(!is3DObjectActive) throw SCENE_FILE_ERROR;
				is3DObjectActive = false;
			}
			else if(strCommand == "numvertices")
			{
				if(is3DObjectActive)
					inputFile >> m_sceneMeshProperties.num_vertices;
				else 
					throw SCENE_FILE_ERROR;

				if(m_sceneMeshProperties.num_vertices <= 0) throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "numnormals")
			{
				if(is3DObjectActive)
					inputFile >> m_sceneMeshProperties.num_normals;
				else 
					throw SCENE_FILE_ERROR;

				if(m_sceneMeshProperties.num_normals <= 0) throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "numtexcoords")
			{
				if(is3DObjectActive)
					inputFile >> m_sceneMeshProperties.num_texCoords;
				else 
					throw SCENE_FILE_ERROR;

				if(m_sceneMeshProperties.num_texCoords <= 0) throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "file")
			{
				if(is3DObjectActive) 
				{
					string fileName;
					inputFile >> fileName;

					SAFE_DELETE_ARRAY(wstrNameC);
					wstrNameC = new WCHAR[fileName.size()+1];

					if(MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, wstrNameC, fileName.size()+1) == 0) {
						ErrorWarning(L"Scene::LoadSceneFromFile-->MultiByteToWideChar");
						throw 'e';
					}

					m_sceneMeshProperties.file = wstring(wstrNameC);

				} else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "rotation")
			{
				float x,y,z;
				inputFile >> x >> y >> z;

				if(is3DObjectActive)
					m_sceneMeshProperties.rot = D3DXVECTOR3(x,y,z);
				else 
					throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "scale")
			{
				inputFile >> m_scale;

				if(m_scale <= 0) throw SCENE_FILE_ERROR;
			}
			else if(strCommand == "showsky")
			{
				int tmp;
				inputFile >> tmp;
				m_showSky = tmp == 0 ? false : true;
			}
			else
			{
				//comando desconocido
			}

			if(inputFile.eof()) break;
			inputFile.ignore(numeric_limits<streamsize>::max(), '\n');		//pasamos a la siguiente linea
			if(inputFile.eof()) break;

			//leemos mas datos del archivo si es que lo que nos queda del bloque presente no alcanza para procesar una nueva línea
			long remainingBytes = static_cast<long> (bytesRead - inputFile.tellg());

			if( (remainingBytes < MAX_LINE_LENGTH) && bytesRead == BUFFERSIZE-1) 
			{
				inputFile.read(readBuffer, remainingBytes);

				if(!ReadFile(hFile, &(readBuffer[remainingBytes]), BUFFERSIZE-remainingBytes-1, &bytesRead, NULL)) {
					ErrorWarning(L"Scene::LoadSceneFromFile-->ReadFile");
					throw 'e';
				}
				//if(bytesRead == 0) break;
				readBuffer[bytesRead+remainingBytes] = NULL;
				inputFile.str(readBuffer);
				inputFile.clear(stringstream::goodbit);
				inputFile.seekg(0);
				bytesRead += remainingBytes;
			}
		}
		hr = S_OK;
	}
	catch (bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		hr = E_FAIL;
	}
	catch (std::length_error &) 
	{
		MiscErrorWarning(LENGTH_ERROR);
		hr = E_FAIL;
	}
	catch (char &) 
	{
		hr = E_FAIL;
	}
	catch (stringstream::failure &) 
	{
		MiscErrorWarning(SS_ERROR);
		hr = E_FAIL;
	}
	catch (SceneFileError &) 
	{
		MiscErrorWarning(MISCSCENEFILE_ERROR);
		hr = E_FAIL;
	}

	// cleanup

	SAFE_DELETE_ARRAY(wstrNameC);

	if(CloseHandle(hFile) == 0) ErrorWarning(L"Scene::LoadSceneFromFile-->CloseHandle");	//hubo un error pero continuamos, porque no es fatal

	return hr;
}

}