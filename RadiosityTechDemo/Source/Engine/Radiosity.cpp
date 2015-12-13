//------------------------------------------------------------------------------------------
// File: Radiosity.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Radiosity.h"

namespace DTFramework
{

Radiosity::Radiosity(const D3DDevicesManager &d3d, const bool exportHemicubes, const bool enableProfiling, const UINT verticesBakedPerDispatch, const UINT numBounces)
: 
VERTICES_BAKED_PER_DISPATCH(max(verticesBakedPerDispatch, (UINT) 1)), PASSES(max(numBounces, (UINT) 1)), 
PARENT_HEMICUBES_TEXTURE_WIDTH(min(PARENT_HEMICUBES_TEXTURE_MAX_WIDTH, VERTICES_BAKED_PER_DISPATCH * HEMICUBE_FACE_SIZE * NUM_HEMICUBE_FACES)),
FACES_PER_ROW(PARENT_HEMICUBES_TEXTURE_WIDTH / HEMICUBE_FACE_SIZE),
FACES_PER_COLUMN( (UINT) ceil( (VERTICES_BAKED_PER_DISPATCH * NUM_HEMICUBE_FACES) / (float) FACES_PER_ROW )   ),

m_d3dManager(d3d), 
m_hemiCubes(0), m_depthStencilBuffer(0),
m_lastPassGIDataSRV(0), m_finalGIDataSRV(0),

m_profiling(enableProfiling), m_timer(d3d), m_timer2(d3d), m_hemicubeRenderingTime(0), m_totalIntegrationTime(0), m_totalAlgorithmTime(0),

m_exportHemicubes(exportHemicubes), m_ready(false)
{

}

Radiosity::~Radiosity()
{
	//no deben liberarse aquí. Eso se hace en el destructor de ImmutableBuffer o WritableBuffer según 
	//si usamos CPURadiosity o GPURadiosity
	/*SAFE_RELEASE(m_finalGIDataSRV);
	SAFE_RELEASE(m_lastPassGIDataSRV);*/

	SAFE_DELETE(m_hemiCubes);
	SAFE_DELETE(m_depthStencilBuffer);

	if(m_outputFile.is_open())
		m_outputFile.close();
}


HRESULT Radiosity::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Radiosity::Init");
		return E_FAIL;
	}

	HRESULT hr;

	if(m_profiling) {
		m_outputFile.open(PROFILING_FILE);
		m_timer.Start();
		m_timer2.Start();
	}

	ComputeVertexWeight();
	
	//crear e inicializar la texture donde renderizaremos los hemicubos
	if((m_hemiCubes = new (std::nothrow) RenderableTexture(m_d3dManager)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	//el formato DXGI_FORMAT_R32G32B32_FLOAT no es soportado por casi ninguna tarjeta D3D11
	if(FAILED(hr = m_hemiCubes->Init(PARENT_HEMICUBES_TEXTURE_WIDTH, FACES_PER_COLUMN * HEMICUBE_FACE_SIZE, false, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, true))) return hr;

	//depth stencil buffer (debe tener el mismo tamaño en texels que la textura donde renderizamos los hemicubos)
	if((m_depthStencilBuffer = new (std::nothrow) Texture2D_NOAA(m_d3dManager, PARENT_HEMICUBES_TEXTURE_WIDTH, FACES_PER_COLUMN * HEMICUBE_FACE_SIZE, 1, 1, 
	                                                             DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_USAGE_DEFAULT, D3D11_BIND_DEPTH_STENCIL)) == NULL)
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED( hr = m_depthStencilBuffer->Init())) return hr;

	return S_OK;
}


HRESULT Radiosity::ProcessScene(Renderer &renderer, Scene &scene, Light &light, const UINT pass)
{
	HRESULT hr;

	for(UINT i=0; i<m_vertices.size(); i += VERTICES_BAKED_PER_DISPATCH) {
		if(FAILED(hr = ProcessVertex(renderer, scene, light, pass, i))) return hr;
	}

	return hr;
}


//------------------------------------------------------------------------------------------
// Renderización de los hemicubos de los VERTICES_BAKED_PER_DISPATCH vértices (o el resto si
// es menor) comenzando desde el vértice con id vertexId.
// La renderización se efectúa con los objetos renderer, scene y light. Luego de finalizar
// se invoca al método de integración.
//------------------------------------------------------------------------------------------
HRESULT Radiosity::ProcessVertex(Renderer &renderer, Scene &scene, Light &light, const UINT pass, const UINT vertexId)
{
	HRESULT hr;

	if(m_profiling)
		m_timer.UpdateForGPU();

	//asignar render target y depth buffer
	ID3D11RenderTargetView *renderTargets[1] = { m_hemiCubes->GetRenderTargetView() };
	m_d3dManager.OMSetRenderTargets(1, renderTargets, m_depthStencilBuffer->GetDepthStencilView());

	//limpiar render target y depth buffer
	const float ambientColor = 0.0f; //max(0.2f - pass * 0.1, 0.0f);
	m_d3dManager.ClearRenderTargetView(renderTargets[0], reinterpret_cast<float *>(&D3DXVECTOR4(ambientColor,ambientColor,ambientColor,ambientColor)) );
	m_d3dManager.ClearDepthStencilView(m_depthStencilBuffer->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//renderizar hemicubo para cada vértice
	for(UINT i=vertexId; i<min(vertexId + VERTICES_BAKED_PER_DISPATCH, m_vertices.size()); ++i) 
	{
		for(UINT face=0; face<5; ++face) 
		{
			const UINT textureNumber = ((i - vertexId) * NUM_HEMICUBE_FACES + face);
			const UINT textureRow = textureNumber / FACES_PER_ROW;
			const UINT textureColumn = textureNumber % FACES_PER_ROW;

			//viewport
			D3D11_VIEWPORT viewport;
			viewport.Width = HEMICUBE_FACE_SIZE;
			viewport.Height = HEMICUBE_FACE_SIZE;
			viewport.TopLeftX = (FLOAT) textureColumn * HEMICUBE_FACE_SIZE;		//coordenadas dentro del render target texture
			viewport.TopLeftY = (FLOAT) textureRow * HEMICUBE_FACE_SIZE;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			m_d3dManager.RSSetViewports(1, &viewport);
			
			//rasterizer state. Rectángulo scissor para no renderizar todo un cubo sino un hemicubo
			D3D11_RECT scissorRect;
			GetFaceScissorRectangle(face, (UINT) viewport.TopLeftX, (UINT) viewport.TopLeftY, scissorRect);
			m_d3dManager.RSSetScissorRects(1, &scissorRect);

			//view y projection matrices
			D3DXMATRIX view;
			VertexCameraMatrix(m_vertices[i], face, view);
			
			D3DXMATRIX projection;
			D3DXMatrixPerspectiveFovLH(&projection,  static_cast<float> (D3DX_PI) / 2.0f, 1.0f, 0.1f, 3500.0f);	
			
			//primer bounce => cielo y geometría sin luz
			if(pass == 0)
				hr = renderer.AuxiliarRenderDepthAndSkybox(scene, light, m_vertices[i].position, view, projection, DEVICE_STATE_RASTER_SOLID_CULLNONE_SCISSOR);
			//segundo bounce (o primero si no hay cielo) => iluminación directa + primer rebote del sky light 
			else if(pass == 1)	
				hr = renderer.Render(scene, &light, m_vertices[i].position, view, projection, m_lastPassGIDataSRV, DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR, false);
			//tercer bounce (o segundo si no hay cielo) => sólo iluminación del rebote anterior
			else
				hr = renderer.Render(scene, NULL, m_vertices[i].position, view, projection, m_lastPassGIDataSRV, DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR);
			
			if(FAILED(hr)) {
				ID3D11RenderTargetView *rtvs[1] = {NULL};
				m_d3dManager.OMSetRenderTargets(0, rtvs, NULL);
				return hr;
			}

			#ifdef DEBUG_GI_GRAPHICS_DATA
				StagingTexture stagingTex(m_d3dManager, PARENT_HEMICUBES_TEXTURE_WIDTH, FACES_PER_COLUMN * HEMICUBE_FACE_SIZE, DXGI_FORMAT_R32G32B32A32_FLOAT);
				if(FAILED(hr = stagingTex.Init())) { 
					ID3D11RenderTargetView *rtvs[1] = {NULL};
					m_d3dManager.OMSetRenderTargets(0, rtvs, NULL);
					return hr;
				}
				const float *rawVB = stagingTex.GetMappedData(m_hemiCubes->GetColorTexture());
				stagingTex.CloseMappedData();
			#endif
		}
	}

	//desbindear render targets
	ID3D11RenderTargetView *rtvs[1] = {NULL};
	m_d3dManager.OMSetRenderTargets(0, rtvs, NULL);

	
	if(m_profiling) {
		m_timer.UpdateForGPU();
		m_hemicubeRenderingTime +=  m_timer.GetTimeElapsed();
	}

	hr = IntegrateHemicubeRadiance(vertexId, min(VERTICES_BAKED_PER_DISPATCH, m_vertices.size() - vertexId), pass);

	return hr;
}

//inverso de la suma de todos los delta form factors
void Radiosity::ComputeVertexWeight()
{
	//suma de los delta form factors en la cara +z
	float totalWeightZ = 0.0f;
	for(UINT y = 0; y < HEMICUBE_FACE_SIZE; ++y) {
		for(UINT x = 0; x < HEMICUBE_FACE_SIZE; ++x) {
			const float u = (static_cast<float>(x) / static_cast<float>(HEMICUBE_FACE_SIZE-1)) * 2.0f - 1.0f;
			const float v = (static_cast<float>(y) / static_cast<float>(HEMICUBE_FACE_SIZE-1)) * 2.0f - 1.0f;

			const float tmp = u * u + v * v + 1.0f;
			const float pixel_weight =	1.0f / (float) (tmp * tmp * D3DX_PI);

			totalWeightZ += (pixel_weight);
		}
	}

	//suma de los delta form factor en una cara lateral
	float totalWeightY = 0.0f;
	for(UINT y = HEMICUBE_FACE_SIZE/2; y < HEMICUBE_FACE_SIZE; ++y) {
		for(UINT x = 0; x < HEMICUBE_FACE_SIZE; ++x) {
			const float u = (static_cast<float>(x) / static_cast<float>(HEMICUBE_FACE_SIZE-1)) * 2.0f - 1.0f;
			const float v = (static_cast<float>(y) / static_cast<float>(HEMICUBE_FACE_SIZE-1)) * 2.0f - 1.0f;

			const float tmp = u * u + v * v + 1.0f;
			const float pixel_weight = abs(v) / (float) (tmp * tmp * D3DX_PI);

			totalWeightY += (pixel_weight);
		}
	}
	
	//inverso de la suma total para que la suma de los deltaFormFactors dé uno
	m_giCalcConstants.vertexWeight = 1.0f/(totalWeightZ + totalWeightY*4.0f);
}

HRESULT Radiosity::PrepareGIVerticesVector(const Mesh &mesh)
{
	HRESULT hr;

	InputLayouts inputLayout(m_d3dManager);

	const D3D11_INPUT_ELEMENT_DESC * const stdLayoutDesc = inputLayout.GetStandardLayoutDesc();
	const UINT stdLayoutNumElements = inputLayout.GetStandardLayoutNumElements();

	UINT positionOffset, normalOffset, tangentOffset;

	//cargar offsets de vectores requeridos en el Vertex
	for(UINT i = 0; i < stdLayoutNumElements; ++i) {
		const D3D11_INPUT_ELEMENT_DESC &element = stdLayoutDesc[i];
		string semantic(element.SemanticName);
		if(semantic == "POSITION")
			positionOffset = element.AlignedByteOffset;
		else if(semantic == "NORMAL")
			normalOffset = element.AlignedByteOffset;
		else if(semantic == "TANGENT")
			tangentOffset = element.AlignedByteOffset;
	}

	//copiar el vertex buffer de la mesh a un staging buffer para que el CPU pueda leerlo
	D3D11_BUFFER_DESC vbDesc;
	mesh.GetVertexBuffer()->GetDesc(&vbDesc);

	StagingBuffer stagingBuffer(m_d3dManager, vbDesc.ByteWidth);
	if(FAILED( hr = stagingBuffer.Init() )) return hr;

	m_d3dManager.CopyResource( stagingBuffer.GetBuffer(), mesh.GetVertexBuffer() );

	//map staging buffer
	D3D11_MAPPED_SUBRESOURCE mapped;
	if( FAILED(hr = m_d3dManager.Map(stagingBuffer.GetBuffer(), 0, D3D11_MAP_READ, 0, &mapped) )) return hr;

	const unsigned char *rawVB = (const unsigned char *) (mapped.pData);

	try {
		//copiar vertices a un vector de vertices (posicion, normal tangente y bitangente)
		const unsigned char *positions = (const unsigned char *)(rawVB + positionOffset);
		const unsigned char *normals = (const unsigned char *)(rawVB + normalOffset);
		const unsigned char *tangents = (const unsigned char *)(rawVB + tangentOffset);

		const UINT numVertices = mesh.GetTotalVertices();
		m_vertices.reserve( numVertices );
		m_vertices.clear();

		for(UINT i=0; i<numVertices; ++i) {
			GIVertex tmp;

			tmp.position = *((D3DXVECTOR3 *) positions);
			tmp.normal = *((D3DXVECTOR3 *) normals);
			tmp.tangent = *((D3DXVECTOR3 *) tangents);

			D3DXVec3Normalize(&(tmp.normal), &(tmp.normal));
			D3DXVec3Normalize(&(tmp.tangent), &tmp.tangent);
			D3DXVec3Cross(&(tmp.bitangent), &(tmp.normal), &(tmp.tangent));				//bitangente. Perpendicular al plano formado por los otros dos vectores
			D3DXVec3Normalize(&(tmp.bitangent), &(tmp.bitangent));
		
			m_vertices.push_back(tmp);

			positions += sizeof(Vertex);
			normals += sizeof(Vertex);
			tangents += sizeof(Vertex);
		}	
	}
	catch (std::bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		hr = E_FAIL;
	}
	catch (std::length_error &) 
	{
		MiscErrorWarning(LENGTH_ERROR);
		hr = E_FAIL;
	}

	m_d3dManager.Unmap(stagingBuffer.GetBuffer(), 0);

	return hr;
}

//------------------------------------------------------------------------------------------
// left y top: origen del rectángulo scissor (en el render target)
// face: índice de la cara del hemicubo. 0,1,2,3,4: +z, +x, -x, +y, -y resp.
//------------------------------------------------------------------------------------------
inline void Radiosity::GetFaceScissorRectangle(const UINT face, const UINT left, const UINT top, D3D11_RECT &scissorRect)
{
	switch(face) 
	{
		case 0:												//+z
			scissorRect.left = left;
			scissorRect.right = left + HEMICUBE_FACE_SIZE;
			scissorRect.top = top;
			scissorRect.bottom = top + HEMICUBE_FACE_SIZE;
			break;
		case 1:												//+x
			scissorRect.left = left;
			scissorRect.right = left + HEMICUBE_FACE_SIZE / 2;
			scissorRect.top = top;
			scissorRect.bottom = top + HEMICUBE_FACE_SIZE;
		break;
		case 2:												//-x
			scissorRect.left = left + HEMICUBE_FACE_SIZE / 2;
			scissorRect.right = left + HEMICUBE_FACE_SIZE;
			scissorRect.top = top;
			scissorRect.bottom = top + HEMICUBE_FACE_SIZE;
		break;
		case 3:												//+y
			scissorRect.left = left;
			scissorRect.right = left + HEMICUBE_FACE_SIZE;
			scissorRect.top = top + HEMICUBE_FACE_SIZE / 2;
			scissorRect.bottom = top + HEMICUBE_FACE_SIZE;
		break;
		case 4:												//-y
			scissorRect.left = left;
			scissorRect.right = left + HEMICUBE_FACE_SIZE;
			scissorRect.top = top;
			scissorRect.bottom = top + HEMICUBE_FACE_SIZE / 2;
		break;
	}
}

//------------------------------------------------------------------------------------------
// Construye view matrix dado un vértice y un índice de cara del hemicubo.
// face: índice de la cara del hemicubo. 0,1,2,3,4: +z, +x, -x, +y, -y resp.
//------------------------------------------------------------------------------------------
inline void Radiosity::VertexCameraMatrix(const GIVertex &vertex, const UINT face, D3DXMATRIX &viewMatrix)
{
	D3DXVECTOR3 x(vertex.tangent.x, vertex.tangent.y, vertex.tangent.z);
	D3DXVECTOR3 y(vertex.bitangent.x, vertex.bitangent.y, vertex.bitangent.z);
	D3DXVECTOR3 z(vertex.normal.x, vertex.normal.y, vertex.normal.z);

	switch(face) 
	{
		case 0:
			viewMatrix(0, 0) = x.x; viewMatrix(0, 1) = x.y; viewMatrix(0, 2) = x.z; viewMatrix(0, 3) = 0.0f;	//fila 0 = x
			viewMatrix(1, 0) = y.x; viewMatrix(1, 1) = y.y; viewMatrix(1, 2) = y.z; viewMatrix(1, 3) = 0.0f;	//fila 1 = y
			viewMatrix(2, 0) = z.x; viewMatrix(2, 1) = z.y; viewMatrix(2, 2) = z.z; viewMatrix(2, 3) = 0.0f;	//fila 2 = z
			break;
		case 1:
			viewMatrix(0, 0) = -z.x; viewMatrix(0, 1) = -z.y; viewMatrix(0, 2) = -z.z; viewMatrix(0, 3) = 0.0f;
			viewMatrix(1, 0) = y.x; viewMatrix(1, 1) = y.y; viewMatrix(1, 2) = y.z; viewMatrix(1, 3) = 0.0f;
			viewMatrix(2, 0) = x.x; viewMatrix(2, 1) = x.y; viewMatrix(2, 2) = x.z; viewMatrix(2, 3) = 0.0f;
			break;
		case 2:
			viewMatrix(0, 0) = z.x; viewMatrix(0, 1) = z.y; viewMatrix(0, 2) = z.z; viewMatrix(0, 3) = 0.0f;
			viewMatrix(1, 0) = y.x; viewMatrix(1, 1) = y.y; viewMatrix(1, 2) = y.z; viewMatrix(1, 3) = 0.0f;
			viewMatrix(2, 0) = -x.x; viewMatrix(2, 1) = -x.y; viewMatrix(2, 2) = -x.z; viewMatrix(2, 3) = 0.0f;
			break;
		case 3:
			viewMatrix(0, 0) = x.x; viewMatrix(0, 1) = x.y; viewMatrix(0, 2) = x.z; viewMatrix(0, 3) = 0.0f;
			viewMatrix(1, 0) = -z.x; viewMatrix(1, 1) = -z.y; viewMatrix(1, 2) = -z.z; viewMatrix(1, 3) = 0.0f;
			viewMatrix(2, 0) = y.x; viewMatrix(2, 1) = y.y; viewMatrix(2, 2) = y.z; viewMatrix(2, 3) = 0.0f;
			break;
		case 4:
			viewMatrix(0, 0) = x.x; viewMatrix(0, 1) = x.y; viewMatrix(0, 2) = x.z; viewMatrix(0, 3) = 0.0f;
			viewMatrix(1, 0) = z.x; viewMatrix(1, 1) = z.y; viewMatrix(1, 2) = z.z; viewMatrix(1, 3) = 0.0f;
			viewMatrix(2, 0) = -y.x; viewMatrix(2, 1) = -y.y; viewMatrix(2, 2) = -y.z; viewMatrix(2, 3) = 0.0f;
			break;
	}

	viewMatrix(3, 0) = vertex.position.x; viewMatrix(3, 1) = vertex.position.y; viewMatrix(3, 2) = vertex.position.z; viewMatrix(3, 3) = 1.0f;

	D3DXMatrixInverse(&viewMatrix, NULL, &viewMatrix);
}


inline static float RoundPixelColorValue(const float value)
{
	float tmp = fabs(value) - floor(fabs(value));

	if(tmp >= 0.5) return ceil(fabs(value));

	return floor(fabs(value));
}

//------------------------------------------------------------------------------------------
// exportar las hemicube faces a un archivo BMP para su visualización.
// hemicubeData --> puntero a los hemicubos en memoria de sistema.
//------------------------------------------------------------------------------------------
void Radiosity::ExportHemicubeFaces(const float * const hemicubeData, const UINT vertexId, const UINT pass) const
{
	_ASSERT(hemicubeData);

	ofstream output;
	output.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	string fileName = "HemicubeFacesVertice" + std::to_string(vertexId) + "Pass" + std::to_string(pass) + ".bmp";

	try 
	{
		output.open(fileName.c_str(), std::ios::binary);	//escribir al archivo BMP usando datos binarios. Muy importante esto último

		const UINT imgWidth = PARENT_HEMICUBES_TEXTURE_WIDTH;
		const UINT imgHeight = FACES_PER_COLUMN * HEMICUBE_FACE_SIZE;

		const UINT rowImgSize = PARENT_HEMICUBES_TEXTURE_WIDTH * 3 + (( (PARENT_HEMICUBES_TEXTURE_WIDTH * 3) % 4 ) != 0 ? 4 - ( (PARENT_HEMICUBES_TEXTURE_WIDTH * 3) % 4 ) : 0) ;
		const UINT pixelArraySize = rowImgSize * imgHeight;
	
		const UINT offsetPixelArray = 54;
		const UINT fileSize = offsetPixelArray + pixelArraySize;

		const UINT headerSize = 40;

		const char *p = (const char *) &fileSize;
		const char *p2 = (const char *) &offsetPixelArray;
		const char *p3 = (const char *) &headerSize;
		const char *p4 = (const char *) &imgWidth;
		const char *p5 = (const char *) &imgHeight;
		const char *p6 = (const char *) &pixelArraySize;


		const char cero = 0;
		const char uno = 1;
		const char bpp = 24;

		//escribir header
		output.write("BM", 2);
		output.write(p, 4);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(p2, 4);
		output.write(p3, 4);
		output.write(p4, 4);
		output.write(p5, 4);
		output.write(&uno, 1);
		output.write(&cero, 1);
		output.write(&bpp, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(p6, 4);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);
		output.write(&cero, 1);


		//escribir pixel array
		for(int i=imgHeight - 1; i>=0; --i) {
			const float * const rowStart = hemicubeData + i * imgWidth * 4;
			for(UINT j=0; j<imgWidth; ++j) {
				const float *currentPixel = rowStart + j * 4;

				//convertir el color de 32 bits a 8 bits
				const unsigned char red = static_cast<unsigned char>( RoundPixelColorValue( (*currentPixel) * 255.0f ) );
				const unsigned char green = static_cast<unsigned char>( RoundPixelColorValue( (*(currentPixel+1)) * 255.0f ) );
				const unsigned char blue = static_cast<unsigned char>( RoundPixelColorValue( (*(currentPixel+2)) * 255.0f ) );

				output.write((char *) &blue, 1);
				output.write((char *) &green, 1);
				output.write((char *) &red, 1);
			}
		}
	}
	catch (std::ofstream::failure &) 
	{
		MiscErrorWarning(IFSTREAM_ERROR, L"Hemicube Export");
	}

	if(output.is_open())
		output.close();
}

}