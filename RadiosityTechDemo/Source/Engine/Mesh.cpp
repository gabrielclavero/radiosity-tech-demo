//------------------------------------------------------------------------------------------
// File: Mesh.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Mesh.h"

namespace DTFramework
{

Mesh::Mesh(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_numAttribTableEntries(0), m_pAttribTable(0), m_mesh(0), m_vertexBuffer(0), m_indexBuffer(0),
  m_totalVertices(0), m_totalFaces(0), m_ready(false)
{
	
}


Mesh::~Mesh()
{
	//m_materials.clear();

	m_vertices.clear();
	m_indices.clear();
	m_attributes.clear();

	SAFE_DELETE_ARRAY(m_pAttribTable);
	SAFE_RELEASE(m_mesh);

	SAFE_DELETE(m_indexBuffer);
	SAFE_DELETE(m_vertexBuffer);
}

//meshFile es el nombre del archivo solamente. No la ruta completa
HRESULT Mesh::Init(const wstring &meshFile,  const UINT numVertices, const UINT numNormals, const UINT numCoords)
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Mesh::Init");
		return E_FAIL;
	}

	m_meshFile = meshFile;

	HRESULT hr;

	//cargar el vertex buffer, index buffer e información de subsets de un archivo .obj
	if(FAILED( hr = LoadGeometryFromOBJ( MESHES_DIRECTORY + meshFile, numVertices, numNormals, numCoords ) ))
		return hr;

	//crear la ID3DX10Mesh
	InputLayouts inputLayout(m_d3dManager);
	const D3D10_INPUT_ELEMENT_DESC *ieDesc = inputLayout.GetMeshLayoutDesc();

	if( FAILED (hr = m_d3dManager.CreateMesh(ieDesc, inputLayout.GetMeshLayoutNumElements(), ieDesc[0].SemanticName,
	                                         static_cast<UINT> ( m_vertices.size() ), static_cast<UINT> ( m_indices.size() / 3 ), D3DX10_MESH_32_BIT, &m_mesh ) ) ) return hr;

	m_totalVertices = static_cast<UINT> ( m_vertices.size() );
	m_totalFaces = static_cast<UINT> ( m_indices.size()  / 3 );

	if(m_totalVertices == 0 || m_totalFaces == 0) {
		MiscErrorWarning(MESHFILE_ERROR, L"Mesh::Init");
		return E_INVALIDARG;
	}

	if(FAILED(hr = CalculateTangents())) return hr;

	// setear vertex data
	if(FAILED( hr = m_mesh->SetVertexData( 0, (void *) ( &m_vertices[0] ) ) )) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::SetVertexData");
		return hr;
	}
	m_vertices.clear();

	// setear index data
	if(FAILED( hr = m_mesh->SetIndexData( (void *) (&m_indices[0]), static_cast<UINT> ( m_indices.size() ) ))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::SetIndexData");
		return hr;
	}
	m_indices.clear();

	// setear attribute data
	if(FAILED( hr = m_mesh->SetAttributeData( (UINT *)&m_attributes[0] ))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::SetAttributeData");
		return hr;
	}
	m_attributes.clear();	//Borramos esto porque no nos sirve luego de aplicar Optimize. En su lugar usamos la tabla de atributos

	//reorganizar los vertices de acuerdo al subset y optimizar la mesh para la cache de vertices de la tarjeta de video
	//Cuando se renderiza la lista de triangulos de la mesh los vertices van a hacer cache hit mas a menudo asi que nos evitamos volver a ejecutar el vertex shader
	if(FAILED( hr = m_mesh->GenerateAdjacencyAndPointReps( 1e-6f ) )) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::GenerateAdjacencyAndPointReps");
		return hr;
	}	
	const UINT a = m_mesh->GetVertexCount();
	if(FAILED( hr = m_d3dManager.OptimizeMesh(m_mesh, D3DX10_MESHOPT_ATTR_SORT | D3DX10_MESHOPT_VERTEX_CACHE, NULL, NULL, a ) )) 
		return hr;
	m_totalVertices = m_mesh->GetVertexCount();


	//cargar tabla de atributos
	if(FAILED(hr = m_mesh->GetAttributeTable( NULL, &m_numAttribTableEntries ))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::GetAttributeTable");
		return hr;
	}

	if(m_numAttribTableEntries == 0) { 
		MiscErrorWarning(MESHFILE_ERROR, L"Mesh::Init");
		return E_INVALIDARG;
	}

	if((m_pAttribTable = new (std::nothrow) D3DX10_ATTRIBUTE_RANGE[m_numAttribTableEntries]) == NULL) {
		MiscErrorWarning(BAD_ALLOC); 
		return E_FAIL;
	}
	if(FAILED(hr = m_mesh->GetAttributeTable( m_pAttribTable, &m_numAttribTableEntries ))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::GetAttributeTable");
		return hr;
	}

	//crear vertex, index buffer de direct3d11 copiando los datos que están en el vertex e index buffer de la ID3DX10Mesh
	//vertex buffer
	ID3DX10MeshBuffer *meshVertexBuffer = NULL;
	void *vertices = NULL;
	SIZE_T size;

	if(FAILED(hr = m_mesh->GetVertexBuffer(0, &meshVertexBuffer))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::GetVertexBuffer");
		return hr;
	}
	if(FAILED(hr = meshVertexBuffer->Map(&vertices, &size))) {  //size = sizeof(Vertex) * m_mesh->GetVertexCount();
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10MeshBuffer::Map");
		SAFE_RELEASE(meshVertexBuffer);
		return hr;
	}
	if((m_vertexBuffer = new (std::nothrow) VertexBuffer(m_d3dManager, size, vertices)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		meshVertexBuffer->Unmap();
		SAFE_RELEASE(meshVertexBuffer);
		return E_FAIL;
	}
	if(FAILED( hr = m_vertexBuffer->Init() )) { 
		meshVertexBuffer->Unmap();
		SAFE_RELEASE(meshVertexBuffer);
		return hr;
	}
	meshVertexBuffer->Unmap();
	SAFE_RELEASE(meshVertexBuffer);

	//index buffer
	ID3DX10MeshBuffer *meshIndexBuffer = NULL;
	void *indices = NULL;
	SIZE_T sizeI = 0;

	if(FAILED(hr = m_mesh->GetIndexBuffer(&meshIndexBuffer))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10Mesh::GetIndexBuffer");
		return  hr;
	}
	if(FAILED(hr = meshIndexBuffer->Map(&indices, &sizeI))) {
		DXGI_D3D_ErrorWarning(hr, L"Mesh::Init --> ID3DX10MeshBuffer::Map");
		SAFE_RELEASE(meshIndexBuffer);
		return hr;
	}
	if((m_indexBuffer = new (std::nothrow) IndexBuffer(m_d3dManager, sizeI, indices)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		meshIndexBuffer->Unmap();
		SAFE_RELEASE(meshIndexBuffer);
		return E_FAIL;
	}
	if(FAILED( hr = m_indexBuffer->Init() )) {
		meshIndexBuffer->Unmap();
		SAFE_RELEASE(meshIndexBuffer);
		return hr;
	}
	meshIndexBuffer->Unmap();
	SAFE_RELEASE(meshIndexBuffer);
						

	//cargar texturas del material
	wstring rutaTextura;
	for(UINT i = 0; i < m_materials.size(); ++i) {
		Material *pMaterial = &(m_materials[i]);
		if(pMaterial->GetDiffuseTextureName().size() > 0 ) {
			rutaTextura = TEXTURES_DIRECTORY + pMaterial->GetDiffuseTextureName();

			//creamos una shader resource desde la imagen 2d almacenada en un archivo en disco para poder leerla desde un shader
			ID3D11ShaderResourceView *srv = (ID3D11ShaderResourceView *) ERROR_RESOURCE_VALUE;
			if(FAILED( hr = m_d3dManager.CreateShaderResourceViewFromFileD3D11(rutaTextura.c_str(), NULL, NULL, &(srv), NULL) )) {
				MessageBox(NULL, pMaterial->GetDiffuseTextureName().c_str(), L"Texture Error", MB_OK);
				return hr;
			}
			pMaterial->SetDiffuseTextureSRV(srv);		//las copias no aumentan las reference count
		}

		if(pMaterial->GetNormalTextureName().size() > 0 ) {
			rutaTextura = TEXTURES_DIRECTORY + pMaterial->GetNormalTextureName();

			//lo mismo para la normal texture
			ID3D11ShaderResourceView *srv = (ID3D11ShaderResourceView*)ERROR_RESOURCE_VALUE;
			if(FAILED( hr = m_d3dManager.CreateShaderResourceViewFromFileD3D11(rutaTextura.c_str(), NULL, NULL, &(srv), NULL) )) {
				MessageBox(NULL, pMaterial->GetNormalTextureName().c_str(), L"Texture Error", MB_OK);
				return hr;
			}
			pMaterial->SetNormalTextureSRV(srv);
		}
	}

	this->SetTechniquesForMaterials();

	m_ready = true;

	return S_OK;
}


HRESULT Mesh::LoadGeometryFromOBJ( const wstring &strFileName, const UINT numVertices, const UINT numNormals, const UINT numCoords )
{
	HRESULT hr = E_FAIL;

	static const int BUFFERSIZE = 4096;
	static const int MAX_LINE_LENGTH = 100;

	vector <D3DXVECTOR3> *positions = NULL;
	vector <D3DXVECTOR2> *texCoords = NULL;
	vector <D3DXVECTOR3> *normals = NULL;

	HANDLE hFile=INVALID_HANDLE_VALUE;

	WCHAR *wstrNameC = NULL;

	try 
	{	
		//almacenamiento temporal para los datos de entrada
		positions = new vector<D3DXVECTOR3>;
		texCoords = new vector<D3DXVECTOR2>;
		normals = new vector<D3DXVECTOR3>;

		positions->reserve(numVertices+1);
		normals->reserve(numNormals+1);
		texCoords->reserve(numCoords+1);
	

		//material por defecto si el archivo no especifica ninguno para el primer subconjunto de triangulos
		m_materials.push_back( Material() );
		m_materials[m_materials.size()-1].SetName();

		//indice que indica cual material esta activo para las faces que estemos cargando en ese momento
		DWORD curSubset = 0;

		//primer elemento en el vector de posiciones que corresponde al objeto 3ds max que estamos procesando 
		//(hay muchos objetos 3D en la escena 3DS Max pero todos serán cargados en la misma Mesh)
		UINT firstIndex = 0;

		string strMaterialFilenameTmp;
		wstring strMaterialFilename;

		// File input
		char readBuffer[BUFFERSIZE] = {0};	//leemos el archivo de a bloques grandes para mejor performance
		DWORD bytesToRead;
		
		if((hFile = CreateFile(strFileName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE) {
			ErrorWarning(L"Mesh::LoadGeometryFromOBJ --> CreateFile");
			throw 'e';
		}

		if(!ReadFile(hFile, readBuffer, BUFFERSIZE-1, &bytesToRead, NULL)) {
			ErrorWarning(L"Mesh::LoadGeometryFromOBJ --> ReadFile");
			throw 'e';
		}

		//pasamos el buffer leído a un stringstream para una cómoda extracción de datos con formato
		stringstream inputFile(stringstream::in | stringstream::out);
		inputFile.exceptions( stringstream::failbit | stringstream::badbit );

		inputFile << readBuffer;
		char c;
		string strCommand;
		while( (c = inputFile.peek()) != EOF && !inputFile.eof() ) 
		{
			if(!isspace( c )) 
			{
				inputFile >> strCommand;
			
				if( strCommand == "#" ) 
				{
					// comentario
				} 
				else if( strCommand == "g" ) //las g indican que lo que sigue es la descripcion de las caras de un nuevo objeto
				{	
					//los vertices que forman parte de un objeto 3d en el archivo .obj no se comparten con los del siguiente objeto, asi que borremos la tabla hash
					for(UINT i=0; i<HASH_TABLE_SIZE; i++)
						m_hashTable[i].clear();

					firstIndex = positions->size();
				}
				else if( strCommand == "v" ) 
				{
					// posicion
					float x, y, z;
					inputFile >> x >> y >> z;
					(*positions).push_back( D3DXVECTOR3( x, y, z ) );
				}
				else if( strCommand == "vt" ) 
				{
					// texCoord
					float u, v;
					inputFile >> u >> v;
					(*texCoords).push_back( D3DXVECTOR2( u, v ) );
				}
				else if( strCommand == "vn" ) 
				{
					// normal
					float x, y, z;
					inputFile >> x >> y >> z;
					(*normals).push_back( D3DXVECTOR3( x, y, z ) );
				}
				else if( strCommand == "f" ) 
				{
					// face
					UINT iPosition, iTexCoord, iNormal;
					VERTEX vertex;

					for( UINT iFace = 0; iFace < 3; iFace++ ) 
					{
						ZeroMemory( &vertex, sizeof( VERTEX ) );

						// formato OBJ utiliza 1-based arrays
						inputFile >> iPosition;
						if(iPosition - 1 < 0 || iPosition - 1 >= positions->size() || inputFile.eof()) 
							throw OBJFILE_ERROR;
				
						vertex.position = (*positions)[ iPosition - 1 ];

						if( '/' == inputFile.peek() ) 
						{
							inputFile.ignore();

							if( '/' != inputFile.peek() ) 
							{
								// texture coordinate
								inputFile >> iTexCoord;
								if(iTexCoord - 1 < 0 || iTexCoord - 1 >= texCoords->size()|| inputFile.eof()) 
									throw OBJFILE_ERROR;

								vertex.texcoord = (*texCoords)[ iTexCoord - 1 ];
							} 

							if( '/' == inputFile.peek() ) 
							{
								inputFile.ignore();

								// vertex normal
								inputFile >> iNormal;
								if(iNormal - 1 < 0 || iNormal - 1 >= normals->size()) 
									throw OBJFILE_ERROR;

								vertex.normal = (*normals)[ iNormal - 1 ];	
							} 
							else 
								throw OBJFILE_ERROR;
						} 
						else 
							throw OBJFILE_ERROR;
	

						//si el actual no es vertice duplicado lo sumamos a la lista de vertices. Y pasamos su indice al array de indices.
						//El arreglo de indices y vertices se convertiran en el index buffer y vertex buffer de la mesh
						DWORD index = AddVertex(iPosition, vertex);
						m_indices.push_back(index);	
					}
					m_attributes.push_back(curSubset);			
				}
				else if( strCommand == "mtllib" ) 
				{
					// biblioteca de material. Sólo una por archivo a lo sumo
					inputFile >> strMaterialFilenameTmp;
				}
				else if( strCommand == "usemtl" ) 
				{
					// Material
					string strNameTmp;
					inputFile >> strNameTmp;

					SAFE_DELETE_ARRAY(wstrNameC);
					wstrNameC = new WCHAR[strNameTmp.size()+1];
					
					if(MultiByteToWideChar(CP_ACP, 0, strNameTmp.c_str(), -1, wstrNameC, strNameTmp.size()+1) == 0) {
						ErrorWarning(L"Mesh::LoadGeometryFromOBJ --> MultiByteToWideChar");
						throw 'e';
					}

					wstring wstrName(wstrNameC);

					//si es un material repetido simplemente devolvemos su indice en el vector de atributos
					bool bFound = false;
					for( UINT iMaterial = 0; iMaterial < m_materials.size(); iMaterial++ ) 
					{
						if( 0 == m_materials[iMaterial].GetName().compare(wstrName) ) 
						{
							bFound = true;
							curSubset = iMaterial;
							break;
						}
					}

					//si no estaba repetido creamos uno nuevo
					if( !bFound ) 
					{
						m_materials.push_back(Material());
						Material *pMaterial = &(m_materials[m_materials.size() -1]);

						if(FAILED(hr = pMaterial->SetName(wstrNameC))) throw 'e';

						curSubset = (DWORD) m_materials.size() - 1;
					}
				}
				else 
				{
					//comando no implementado o no aceptado
				}
			}
			if(inputFile.eof()) break;

			inputFile.ignore(numeric_limits<streamsize>::max(), '\n');		//pasamos a la siguiente linea

			if(inputFile.eof()) break;

			//leemos mas datos del archivo .obj si es que lo que nos queda del bloque presente no alcanza para procesar una nueva línea
			long remainingBytes = static_cast<long> (bytesToRead - inputFile.tellg());

			if( (remainingBytes < MAX_LINE_LENGTH) && bytesToRead == BUFFERSIZE-1) 
			{
				inputFile.read(readBuffer, remainingBytes);

				if(!ReadFile(hFile, &(readBuffer[remainingBytes]), BUFFERSIZE-remainingBytes-1, &bytesToRead, NULL)) {
					ErrorWarning(L"Mesh::LoadGeometryFromOBJ --> ReadFile");
					throw 'e';
				}
				//if(bytesToRead == 0) break;
				readBuffer[bytesToRead+remainingBytes] = NULL;
				inputFile.str(readBuffer);
				inputFile.clear(stringstream::goodbit);
				inputFile.seekg(0);
				bytesToRead += remainingBytes;
			}
		}

		// Si encontramos un archivo .mtl asociado lo leemos ahora. Notar que solo puede haber un .mtl asociado
		if( strMaterialFilenameTmp.length() > 0 ) 
		{
			SAFE_DELETE_ARRAY(wstrNameC);
			wstrNameC = new WCHAR[strMaterialFilenameTmp.size()+1];

			if(MultiByteToWideChar(CP_ACP, 0, strMaterialFilenameTmp.c_str(), -1, wstrNameC, strMaterialFilenameTmp.size()+1) == 0) {
				ErrorWarning(L"Mesh::LoadGeometryFromOBJ --> MultiByteToWideChar");
				throw 'e';
			}
			if(FAILED( hr = LoadMaterialsFromMTL(MTLS_DIRECTORY + wstring(wstrNameC)) )) {
				MessageBox(NULL, L"Error en la carga del material library", L"Error", MB_OK);
				throw 'e';
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
	catch (OBJMTLError &) 
	{
		MiscErrorWarning(MESHFILE_ERROR);
		hr = E_FAIL;
	}


	// clean up
	SAFE_DELETE(positions); 
	SAFE_DELETE(texCoords);
	SAFE_DELETE(normals);

	SAFE_DELETE_ARRAY(wstrNameC);

	if(hFile != INVALID_HANDLE_VALUE) {
		if(CloseHandle(hFile) == 0) ErrorWarning(L"Mesh::LoadGeometryFromOBJ --> CloseHandle");	//hubo un error pero continuamos, porque no es fatal
	}

	return hr;
}

//Decidimos si agregar el vertice vertex al vector de vertices que se convertirá en el vertexbuffer. Pero debemos evitar agregar vertices duplicados
//por eso se verifica si ya lo hicimos, inspeccionando la tabla hash de los vértices ya agregados.
DWORD Mesh::AddVertex(const UINT a, const Vertex &vertex)
{
	const UINT hash = a % HASH_TABLE_SIZE;

	for(UINT i=0; i<m_hashTable[hash].size(); ++i) 
	{
		if(m_vertices[m_hashTable[hash][i]].position == vertex.position  &&
		   m_vertices[m_hashTable[hash][i]].texcoord == vertex.texcoord &&
		   m_vertices[m_hashTable[hash][i]].normal == vertex.normal) 
				return m_hashTable[hash][i]; 
	}

	const UINT s = static_cast<UINT> ( m_vertices.size() );

	m_vertices.push_back(vertex);	//insertar el vértice en la lista de vertices global

	m_hashTable[hash].push_back(s);	//insertar el índice en la tabla hash

	return (DWORD) s;
}

HRESULT Mesh::LoadMaterialsFromMTL( const wstring &strFileName )
{
	HRESULT hr;

	static const int BUFFERSIZE = 4096;
	static const int MAX_LINE_LENGTH = 100;

	// File input
	string strCommand;
	char readBuffer[BUFFERSIZE] = {0};
	WCHAR *wstrNameC = NULL;

	DWORD bytesToRead;
	HANDLE hFile;

	if((hFile = CreateFile(strFileName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)  {
		ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> CreateFile");
		return E_FAIL;
	}

	try 
	{
		if(!ReadFile(hFile, readBuffer, BUFFERSIZE-1, &bytesToRead, NULL)) {
			ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> ReadFile");
			throw 'e';
		}

		stringstream inputFile(stringstream::in | stringstream::out);
		inputFile.exceptions( std::stringstream::failbit | std::stringstream::badbit );

		inputFile << readBuffer;

		Material *pMaterial = NULL;

		while( !inputFile.eof() && inputFile.peek() != EOF ) 
		{
			while(isspace(inputFile.peek()) && !inputFile.eof()) inputFile.ignore();
			if(inputFile.eof()) break;

			inputFile >> strCommand;

			if( strCommand == "newmtl" ) 
			{
				// cambiar de material activo
				string strNameTmp;
				inputFile >> strNameTmp;

				SAFE_DELETE_ARRAY(wstrNameC);
				wstrNameC = new WCHAR[strNameTmp.size()+1];

				if(MultiByteToWideChar(CP_ACP, 0, strNameTmp.c_str(), -1, wstrNameC, strNameTmp.size()+1) == 0) {
					ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> MultiByteToWideChar");
					throw 'e';
				}

				//los materiales ya fueron creados en LoadGeometryFromObj aquí los buscamos por nombre y les completamos sus settings
				pMaterial = NULL;
				for( UINT i = 0; i < m_materials.size(); i++ ) {
					Material *pCurMaterial = &(m_materials[i]);
					if( 0 == pCurMaterial->GetName().compare( wstrNameC ) ) {
						pMaterial = pCurMaterial;
						break;
					}
				}
			}

			// El resto de los comandos depende del material activo
			if( pMaterial == NULL ) //si el material no fue usado en la escena no nos molestamos en leer sus datos
			{	
				if(inputFile.eof()) break;
				inputFile.ignore(numeric_limits<streamsize>::max(), '\n');		//pasamos a la siguiente linea
				if(inputFile.eof()) break;
				continue;
			}

			if(strCommand == "Ka" )
			{
				// ambient color
				float r, g, b;
				inputFile >> r >> g >> b;
				pMaterial->SetAmbient( D3DXVECTOR3( r, g, b ) );
			}
			else if( strCommand == "Kd" )
			{
				// diffuse color
				float r, g, b;
				inputFile >> r >> g >> b;
				pMaterial->SetDiffuse( D3DXVECTOR3( r, g, b ) );
			}
			else if( strCommand == "Ks" )
			{
				// specular color
				float r, g, b;
				inputFile >> r >> g >> b;
				pMaterial->SetSpecular( D3DXVECTOR3( r, g, b ) );
			}
			else if( strCommand == "d" )
			{
				// alpha
				float fAlpha;
				inputFile >> fAlpha;
				pMaterial->SetAlpha(fAlpha);
			}
			else if( strCommand == "Ns" )
			{
				// shininess
				float shininess;
				inputFile >> shininess;
				pMaterial->SetShininess(shininess);
			}
			else if( strCommand == "illum" )
			{
				// specular on/off
				int illumination;
				inputFile >> illumination;
				pMaterial->SetbSpecular( illumination == 2 );
			}			
			else if( strCommand == "map_Kd" )
			{
				// texture
				string strTextureTmp;
				inputFile >> strTextureTmp;

				SAFE_DELETE_ARRAY(wstrNameC);
				wstrNameC = new WCHAR[strTextureTmp.size()+1];

				if(MultiByteToWideChar(CP_ACP, 0, strTextureTmp.c_str(), -1, wstrNameC, strTextureTmp.size()+1) == 0) {
					ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> MultiByteToWideChar");
					throw 'e';
				}
				pMaterial->SetDiffuseTextureName(wstring(wstrNameC));
			} 
			else if( strCommand == "bump" )
			{
				//normal texture
				string strTextureTmp;
				inputFile >> strTextureTmp;

				SAFE_DELETE_ARRAY(wstrNameC);
				wstrNameC = new WCHAR[strTextureTmp.size()+1];

				if(MultiByteToWideChar(CP_ACP, 0, strTextureTmp.c_str(), -1, wstrNameC, strTextureTmp.size()+1) == 0) {
					ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> MultiByteToWideChar");
					throw 'e';
				}
				pMaterial->SetNormalTextureName(wstring(wstrNameC));
			}
			else
			{
				// comando no implementado o no reconocido
			}


			if(inputFile.eof()) break;
			inputFile.ignore(numeric_limits<streamsize>::max(), '\n');		//pasamos a la siguiente linea
			if(inputFile.eof()) break;


			long remainingBytes = (long) (bytesToRead - inputFile.tellg());

			if( (remainingBytes < MAX_LINE_LENGTH) && bytesToRead == BUFFERSIZE-1) 
			{
				inputFile.read(readBuffer, remainingBytes);

				if(!ReadFile(hFile, &(readBuffer[remainingBytes]), BUFFERSIZE-remainingBytes-1, &bytesToRead, NULL)) {
					ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> ReadFile");
					throw 'e';
				}
				//if(bytesToRead == 0) break;
				readBuffer[bytesToRead+remainingBytes] = NULL;
				inputFile.str(readBuffer);
				inputFile.clear(stringstream::goodbit);
				inputFile.seekg(0);
				bytesToRead += remainingBytes;
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
	catch (OBJMTLError &) 
	{
		MiscErrorWarning(MATERIALFILE_ERROR);
		hr = E_FAIL;
	}

	// cleanup
	SAFE_DELETE_ARRAY(wstrNameC);

	if(CloseHandle(hFile) == 0) ErrorWarning(L"Mesh::LoadMaterialsFromMTL --> CloseHandle");	//hubo un error pero continuamos, porque no es fatal

	return hr;
}

void Mesh::SetTechniquesForMaterials()
{
	// almacenamos el nombre de la technique apropiada para cada material basandonos en sus propiedades
	for(UINT i = 0; i < GetNumMaterials(); ++i) {
		Material *pMaterial = &(m_materials[i]);

		string strTechnique;

		if(pMaterial->GetDiffuseTextureName().size() > 0)
			strTechnique = "Diffuse";
		
		if(pMaterial->GetNormalTextureName().size() > 0)
			strTechnique += "Normal";

		if(pMaterial->GetLightProperties().bSpecular)
			strTechnique += "Specular";

		if(strTechnique.length() == 0)
			strTechnique = "None";

		pMaterial->SetTechniqueName( strTechnique );
	}
}


HRESULT Mesh::CalculateTangents()
{
	//http://www.terathon.com/code/tangent.html

	HRESULT hr = S_OK;

	D3DXVECTOR3 *tan1 = NULL;
	D3DXVECTOR3 *tan2 = NULL;

	try
	{
		tan1 = new D3DXVECTOR3[m_totalVertices * 2];
		tan2 = tan1 + m_totalVertices;
		ZeroMemory(tan1, sizeof(D3DXVECTOR3) * m_totalVertices * 2);

		for(UINT i=0; i<m_indices.size(); i+=3) 
		{
			const D3DXVECTOR3 &v1 = m_vertices[m_indices[i]].position;
			const D3DXVECTOR3 &v2 = m_vertices[m_indices[i+1]].position;
			const D3DXVECTOR3 &v3 = m_vertices[m_indices[i+2]].position;

			const D3DXVECTOR2 &w1 = m_vertices[m_indices[i]].texcoord;
			const D3DXVECTOR2 &w2 = m_vertices[m_indices[i+1]].texcoord;
			const D3DXVECTOR2 &w3 = m_vertices[m_indices[i+2]].texcoord;

			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;

			float s1 = w2.x - w1.x;
			float s2 = w3.x - w1.x;
			float t1 = w2.y - w1.y;
			float t2 = w3.y - w1.y;

			float r = 1.0f / (s1 * t2 - s2 * t1);
			D3DXVECTOR3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
			D3DXVECTOR3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

			tan1[m_indices[i]] += sdir;
			tan1[m_indices[i+1]] += sdir;
			tan1[m_indices[i+2]] += sdir;

			tan2[m_indices[i]] += tdir;
			tan2[m_indices[i+1]] += tdir;
			tan2[m_indices[i+2]] += tdir;
		}

		for(UINT i=0; i<m_totalVertices; ++i) 
		{
			const D3DXVECTOR3 &t = tan1[i];

			m_vertices[i].tangent.x = t.x;
			m_vertices[i].tangent.y = t.y;
			m_vertices[i].tangent.z = t.z;
		}
	}
	catch (bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		hr = E_FAIL;
	}

	SAFE_DELETE_ARRAY(tan1);

	return hr;
}

HRESULT Mesh::Render(const UINT subset) const
{
	_ASSERT(m_ready);

	_ASSERT(subset < m_numAttribTableEntries);

	if(!m_ready || subset >= m_numAttribTableEntries) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Mesh::Render");
		return E_FAIL;
	}

	//primero bindeamos el index y vertex buffer correspondiente al device context
	m_d3dManager.IASetIndexBuffer(m_indexBuffer->GetBuffer(), DXGI_FORMAT_R32_UINT, 0);

	const UINT stride = sizeof(Vertex);
	const UINT offset = 0;
	ID3D11Buffer *tmpBuffer = m_vertexBuffer->GetBuffer();
	m_d3dManager.IASetVertexBuffers(0, 1, &tmpBuffer, &stride, &offset);

	//dibujar
	D3DX10_ATTRIBUTE_RANGE *p = &(m_pAttribTable[subset]);
	m_d3dManager.DrawIndexed(p->FaceCount * 3, p->FaceStart * 3, 0);

	return S_OK;
}

}