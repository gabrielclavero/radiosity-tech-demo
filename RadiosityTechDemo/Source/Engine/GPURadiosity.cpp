//------------------------------------------------------------------------------------------
// File: GPURadiosity.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


#include "GPURadiosity.h"

namespace DTFramework
{

GPURadiosity::GPURadiosity(const D3DDevicesManager &d3d, const bool exportHemicubes, const bool enableProfiling, 
                           const UINT verticesBakedPerDispatch, const UINT verticesBakedPerDispatch2, const UINT numBounces)
: 
Radiosity(d3d, exportHemicubes, enableProfiling, verticesBakedPerDispatch, numBounces),

VERTICES_BAKED_PER_DISPATCH_2(max(verticesBakedPerDispatch2, (UINT) 1)),

m_partialIntegrationBuffer(0),
m_giCalcStep1Shader(d3d), m_giCalcStep2Shader(d3d), m_addPassesShader(d3d),
m_giCalcStep1ComputeShader(0), m_giCalcStep2ComputeShader(0), m_addPassesComputeShader(0), 
m_verticesReadyForStepBMultiplier(0), m_verticesBakedInCurrentPass(0), m_totalVertices(0),
m_giCalcConstantsBuffer(0), m_UVConstantsBuffer(0), 
m_profiler(d3d), m_addPassesTime(0)
{
	for(UINT i=0; i<2; ++i) {
		m_currentAndLastPassGIData[i] = 0;
		m_GITempData[i] = 0;
	}

}

GPURadiosity::~GPURadiosity()
{
	SAFE_DELETE(m_partialIntegrationBuffer);

	for(UINT i=0; i<2; ++i) {
		SAFE_DELETE( m_currentAndLastPassGIData[i] );
		SAFE_DELETE( m_GITempData[i] );
	}

	SAFE_DELETE(m_giCalcConstantsBuffer);
	SAFE_DELETE(m_UVConstantsBuffer);
}

HRESULT GPURadiosity::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"GPURadiosity::Init");
		return E_FAIL;
	}

	//la máxima cantidad para cada dimensión de un dispatch no puede superar ese valor. Aquí verificamos que las elecciones que hicimos no sobrepasen este límite
	if(VERTICES_BAKED_PER_DISPATCH * VERTICES_BAKED_PER_DISPATCH_2 > D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION) {
		MiscErrorWarning(EXCEEDED_MAX_DISPATCH_DIMENSION);
		return E_FAIL;
	}

	HRESULT hr;
	
	if(FAILED(hr = Radiosity::Init())) return hr;

	if(m_profiling) {
		if(FAILED(hr = m_profiler.Init(SHADERS_DIRECTORY + PROFILER_SETTINGS))) return hr;
	}

	if(FAILED(hr = CompileComputeShaders())) return hr;

	m_ready = true;

	return S_OK;
}

HRESULT GPURadiosity::ComputeGIDataForScene(Renderer &renderer, Scene &scene, Light &light)
{
	_ASSERT(m_ready);

	if(!m_ready || !scene.GetSceneMesh()) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"GPURadiosity::ComputeGIDataForScene");
		return E_FAIL;
	}

	m_totalVertices = scene.GetSceneMesh()->GetTotalVertices();

	if(m_totalVertices <= 0) {
		MiscErrorWarning(INVALID_PARAMETER, L"GPURadiosity::ComputeGIDataForScene");
		return E_INVALIDARG;
	}

	HRESULT hr;

	if(m_profiling) {
		m_hemicubeRenderingTime = 0;
		m_totalIntegrationTime = 0;
		m_totalAlgorithmTime = 0;
		m_addPassesTime = 0;
		m_profiler.ResetCounters();

		m_timer2.UpdateForGPU();
	}

	//preparar buffers en GPU
	if(FAILED(hr = PrepareGPUAlgorithmBuffers(*(scene.GetSceneMesh())))) return hr;

	//preparar vector de vértices GI creados en base a los vértices del vertex buffer
	if(FAILED(hr = PrepareGIVerticesVector(*(scene.GetSceneMesh())))) return hr;

	//las pasadas, o iteraciones, representan el numero de veces que calculamos el rebote de la luz. Desde que sale de su origen.
	const bool showSky =  scene.ShowSky();
	const UINT numPasses = showSky ? PASSES : PASSES+1;

	for(UINT pass=0; pass < numPasses; ++pass) 
	{
		m_verticesBakedInCurrentPass = 0;
		if(!showSky && pass == 0) continue;	//no consideramos a la luz del skybox para el GI si no hay cielo en la escena

		if(FAILED(hr = ProcessScene(renderer, scene, light, pass))) return hr;
			
		if( (pass == 0 && showSky) || (pass == 1 && !showSky) ) 
		{
			//en el primer pass copiamos el srv directamente
			m_finalGIDataSRV = m_currentAndLastPassGIData[pass % 2]->GetShaderResourceView();
			m_lastPassGIDataSRV = m_currentAndLastPassGIData[pass % 2]->GetShaderResourceView();

			//DEBUG
			#ifdef DEBUG_GI_GRAPHICS_DATA
				StagingBuffer stagingBuffer(m_d3dManager,  m_currentAndLastPassGIData[pass % 2]->GetByteWidth());
				if(FAILED(hr = stagingBuffer.Init())) return hr;

				const float *rawVB = stagingBuffer.GetMappedData(m_currentAndLastPassGIData[pass % 2]->GetBuffer());
				stagingBuffer.CloseMappedData();
			#endif
		}
		else 
		{
			//sumar esta pasada con la suma parcial de las anteriores
			if(FAILED( hr = AddPasses(pass) )) return hr;

			//srv de la suma parcial. En la última iteración (en el último rebote) queda la suma final
			m_finalGIDataSRV = m_GITempData[pass % 2]->GetShaderResourceView();

			//datos GI de la última pasada solamente. La utilizamos para iluminar el siguiente rebote
			m_lastPassGIDataSRV = m_currentAndLastPassGIData[pass % 2]->GetShaderResourceView();

			#ifdef DEBUG_GI_GRAPHICS_DATA
				StagingBuffer stagingBuffer(m_d3dManager,  m_GITempData[pass % 2]->GetByteWidth());
				if(FAILED(hr = stagingBuffer.Init())) return hr;

				const float *rawVB = stagingBuffer.GetMappedData(m_GITempData[pass % 2]->GetBuffer());
				stagingBuffer.CloseMappedData();
			#endif
		} 
	}

	if(m_profiling) {
		m_timer2.UpdateForGPU();
		m_totalAlgorithmTime = m_timer2.GetTimeElapsed();

		m_outputFile << "RESULTS:" << endl << endl;
		m_outputFile << "Vertices in Scene:\t\t\t" << m_vertices.size() << endl;
		m_outputFile << "Hemicubes' Total Rendering Time:\t" << m_hemicubeRenderingTime << " seconds." << endl;
		m_outputFile << "Add Passes Total Time:\t\t\t" << m_addPassesTime << " seconds." << endl;
		m_outputFile << "Radiosity Algorithm Total Time:\t\t" << m_totalAlgorithmTime << " seconds." << endl;

		m_profiler.EndProfiling(m_outputFile);
	}

	return S_OK;
}

HRESULT GPURadiosity::IntegrateHemicubeRadiance(const UINT vertexId, const UINT verticesBaked, const UINT pass)
{
	HRESULT hr;

	if(m_profiling)
		m_timer.UpdateForGPU();

	//shaders. Solo usaremos el ComputeShader aquí
	m_d3dManager.VSSetShader(NULL, NULL, 0);
	m_d3dManager.GSSetShader(NULL, NULL, 0);
	m_d3dManager.DSSetShader(NULL, NULL, 0);
	m_d3dManager.HSSetShader(NULL, NULL, 0);
	m_d3dManager.PSSetShader(NULL, NULL, 0);
	m_d3dManager.CSSetShader(m_giCalcStep1ComputeShader, NULL, 0);


	//pasar los hemicubos al compute shader
	ID3D11ShaderResourceView *radianceSRView[1] = { m_hemiCubes->GetColorTexture() };

	if(m_exportHemicubes) {
		StagingTexture stagingTex(m_d3dManager, PARENT_HEMICUBES_TEXTURE_WIDTH, FACES_PER_COLUMN * HEMICUBE_FACE_SIZE, DXGI_FORMAT_R32G32B32A32_FLOAT);
		if(FAILED(hr = stagingTex.Init())) return hr;

		const float *rawVB = stagingTex.GetMappedData(m_hemiCubes->GetColorTexture());

		ExportHemicubeFaces(rawVB, vertexId, pass);

		stagingTex.CloseMappedData();
	}

	//asignar constantes
	m_giCalcConstants.vertexId = vertexId - m_verticesReadyForStepBMultiplier * VERTICES_BAKED_PER_DISPATCH;
	m_giCalcConstants.verticesSkipped = m_verticesReadyForStepBMultiplier * VERTICES_BAKED_PER_DISPATCH;

	if(FAILED( hr = m_giCalcConstantsBuffer->Update( (void *) &m_giCalcConstants, sizeof(GI_Calculation_Constants) ) )) return hr;

	ID3D11Buffer *bufferArray[2] = {m_giCalcConstantsBuffer->GetBuffer(), m_UVConstantsBuffer->GetBuffer()};
	m_d3dManager.CSSetConstantBuffers(0, 2, bufferArray);

	//asignar datos de entrada (textura de hemicubos)
	m_d3dManager.CSSetShaderResources(0, 1, radianceSRView);

	//asignar el buffer de salida
	ID3D11UnorderedAccessView *output[1] = { m_partialIntegrationBuffer->GetUnorderedAccessView() };
	m_d3dManager.CSSetUnorderedAccessViews(0, 1, output, NULL);

	//Primer Dispatch
	if(m_profiling)
		m_profiler.StartEvent(0, (UINT) DISPATCH_DIMENSION_X * (UINT) ceil(verticesBaked / 2));

	m_d3dManager.Dispatch(DISPATCH_DIMENSION_X, (UINT) ceil(verticesBaked / 2), 1);

	if(m_profiling)
		m_profiler.EndEvent();

	//actualizar cantidad de vértices listos para segundo dispatch
	m_verticesReadyForStepBMultiplier += 1;
	m_verticesBakedInCurrentPass += verticesBaked;

	//lanzar segundo dispatch si se ha alcanzado VERTICES_BAKED_PER_DISPATCH_2 o si es el último batch en este pass
	if(m_verticesReadyForStepBMultiplier == VERTICES_BAKED_PER_DISPATCH_2
		|| m_verticesBakedInCurrentPass == m_totalVertices ) 
	{
		//asignar compute shader
		m_d3dManager.CSSetShader(m_giCalcStep2ComputeShader, NULL, 0);

		//asignar buffer de salida
		output[0] = m_currentAndLastPassGIData[pass % 2]->GetUnorderedAccessView();
		m_d3dManager.CSSetUnorderedAccessViews(0, 1, output, NULL);

		//asignar buffer de entrada
		ID3D11ShaderResourceView *input[1] = { m_partialIntegrationBuffer->GetShaderResourceView() };
		m_d3dManager.CSSetShaderResources(0, 1, input);

		//Segundo Dispatch
		if(m_profiling)
			m_profiler.StartEvent(1, (m_verticesReadyForStepBMultiplier - 1) * VERTICES_BAKED_PER_DISPATCH + verticesBaked);

		m_d3dManager.Dispatch((m_verticesReadyForStepBMultiplier - 1) * VERTICES_BAKED_PER_DISPATCH + verticesBaked, 1, 1);

		if(m_profiling)
			m_profiler.EndEvent();
	
		//resetear contador
		m_verticesReadyForStepBMultiplier = 0;
	}

	//desbindeamos buffers
	ID3D11ShaderResourceView *input[1] = {NULL};
	m_d3dManager.CSSetShaderResources(0, 1, input);

	output[0] = NULL;
	m_d3dManager.CSSetUnorderedAccessViews(0, 1, output, NULL);

	bufferArray[0] = NULL; bufferArray[1] = NULL;
	m_d3dManager.CSSetConstantBuffers(0, 2, bufferArray);


	if(m_profiling) {
		m_timer.UpdateForGPU();
		m_totalIntegrationTime += m_timer.GetTimeElapsed();
	}

	return S_OK;
}

HRESULT GPURadiosity::AddPasses(const UINT pass)
{
	HRESULT hr;

	if(m_profiling)
		m_timer.UpdateForGPU();

	//shaders
	m_d3dManager.VSSetShader(NULL , NULL, 0);
	m_d3dManager.PSSetShader(NULL, NULL, 0);
	m_d3dManager.GSSetShader(NULL, NULL, 0);
	m_d3dManager.DSSetShader(NULL, NULL, 0);
	m_d3dManager.HSSetShader(NULL, NULL, 0);
	m_d3dManager.CSSetShader(m_addPassesComputeShader, NULL, 0);

	//constantes
	const UINT numElements = m_currentAndLastPassGIData[pass % 2]->GetNumElements();
	m_giCalcConstants.numVertices = numElements;

	if(FAILED( hr = m_giCalcConstantsBuffer->Update( (void *) &m_giCalcConstants, sizeof(GI_Calculation_Constants)) )) return hr;

	ID3D11Buffer *bufferArray[1] = {m_giCalcConstantsBuffer->GetBuffer()};
	m_d3dManager.CSSetConstantBuffers(0, 1, bufferArray);
		
	//buffers de entrada
	ID3D11ShaderResourceView* input[2];
	input[0] = m_finalGIDataSRV;
	input[1] = m_currentAndLastPassGIData[pass % 2]->GetShaderResourceView();
	m_d3dManager.CSSetShaderResources(0, 2, input);

	//buffers de salida
	ID3D11UnorderedAccessView* output[1] = { m_GITempData[pass % 2]->GetUnorderedAccessView() };
	m_d3dManager.CSSetUnorderedAccessViews(0, 1, output, NULL);

	//lanzar dispatch
	UINT numGroups = numElements / NUM_THREADS_FOR_FINAL_STEP;
	if(numElements % NUM_THREADS_FOR_FINAL_STEP > 0) ++numGroups;
	m_d3dManager.Dispatch(numGroups, 1, 1);

	//desbindear
	ID3D11ShaderResourceView* inputBuffers[2] = { NULL };
	m_d3dManager.CSSetShaderResources(0, 2, inputBuffers);

	ID3D11UnorderedAccessView* outputBuffer[1] = { NULL };
	m_d3dManager.CSSetUnorderedAccessViews(0, 1, outputBuffer, NULL);

	bufferArray[0] = NULL;
	m_d3dManager.CSSetConstantBuffers(0, 1, bufferArray);

	if(m_profiling) {
		m_timer.UpdateForGPU();
		m_addPassesTime += m_timer.GetTimeElapsed();
	}

	return S_OK;
}

HRESULT GPURadiosity::PrepareGPUAlgorithmBuffers(const Mesh &sceneMesh)
{
	HRESULT hr;

	//borrar objetos anteriores
	for(UINT i=0; i<2; ++i) {
		SAFE_DELETE(m_currentAndLastPassGIData[i]);
		SAFE_DELETE(m_GITempData[i])
	}
	SAFE_DELETE(m_partialIntegrationBuffer);
	SAFE_DELETE(m_giCalcConstantsBuffer);
	SAFE_DELETE(m_UVConstantsBuffer);

	//constant buffer 0. Datos por dispatch
	const UINT remainderTmp = (sizeof(GI_Calculation_Constants) % 16) == 0 ? 16 : (sizeof(GI_Calculation_Constants) % 16);
	const UINT byteWidth = static_cast<UINT>(sizeof(GI_Calculation_Constants) + 16 - remainderTmp);	//float4 boundary
	if((m_giCalcConstantsBuffer = new (std::nothrow) ConstantBuffer(m_d3dManager, byteWidth)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED( hr = m_giCalcConstantsBuffer->Init() )) return hr;

	//constant buffer 1. UV Function
	float uvFunction[HEMICUBE_FACE_SIZE*4];
	for(UINT i=0; i<HEMICUBE_FACE_SIZE; ++i) {
		uvFunction[i*4] = (static_cast<float>(i) / static_cast<float>(HEMICUBE_FACE_SIZE-1)) * 2.0f - 1.0f;
		uvFunction[i*4+1] = 0.0f;
		uvFunction[i*4+2] = 0.0f;
		uvFunction[i*4+3] = 0.0f;
	}
	if((m_UVConstantsBuffer = new (std::nothrow) ConstantBuffer(m_d3dManager, HEMICUBE_FACE_SIZE*4*4, (void *) uvFunction, false )) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED( hr = m_UVConstantsBuffer->Init() )) return hr;
	

	//inicializar el buffer de integración parcial.
	if((m_partialIntegrationBuffer = new (std::nothrow) 
	                                     WritableBuffer(m_d3dManager, 16 * DISPATCH_DIMENSION_X * VERTICES_BAKED_PER_DISPATCH * VERTICES_BAKED_PER_DISPATCH_2, 
	                                                    DXGI_FORMAT_R32G32B32A32_FLOAT, 
	                                                    DISPATCH_DIMENSION_X * VERTICES_BAKED_PER_DISPATCH * VERTICES_BAKED_PER_DISPATCH_2)) == NULL) 
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED( hr = m_partialIntegrationBuffer->Init() )) return hr;

	//crear los buffers requeridos en memoria de video
	//por cada vértice guardaremos la irradiancia (un sólo valor en RGBA, 4 bytes por cada canal) que proviene de la radiancia de los objetos y el cielo circundante
	if((m_currentAndLastPassGIData[0] = new (std::nothrow) 
	                                        WritableBuffer(m_d3dManager, 16 * sceneMesh.GetTotalVertices(), 
	                                                       DXGI_FORMAT_R32G32B32A32_FLOAT, sceneMesh.GetTotalVertices())) == NULL) 
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if((m_GITempData[0] = new (std::nothrow) 
	                           WritableBuffer(m_d3dManager, 16 * sceneMesh.GetTotalVertices(), 
	                                          DXGI_FORMAT_R32G32B32A32_FLOAT, sceneMesh.GetTotalVertices())) == NULL)
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if((m_GITempData[1] = new (std::nothrow) 
	                           WritableBuffer(m_d3dManager, 16 * sceneMesh.GetTotalVertices(), 
	                                          DXGI_FORMAT_R32G32B32A32_FLOAT, sceneMesh.GetTotalVertices())) == NULL)
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if((m_currentAndLastPassGIData[1] = new (std::nothrow) 
	                                         WritableBuffer(m_d3dManager, 16 * sceneMesh.GetTotalVertices(), 
	                                                        DXGI_FORMAT_R32G32B32A32_FLOAT, sceneMesh.GetTotalVertices())) == NULL)
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	
	if(FAILED( hr = m_currentAndLastPassGIData[0]->Init() )) return hr;
	if(FAILED( hr = m_currentAndLastPassGIData[1]->Init() )) return hr;
	if(FAILED( hr = m_GITempData[0]->Init() )) return hr;
	if(FAILED( hr = m_GITempData[1]->Init() )) return hr;

	return S_OK;
}

HRESULT GPURadiosity::CompileComputeShaders()
{
	HRESULT hr;

	std::stringstream ss;
	ss.exceptions(std::stringstream::failbit | std::stringstream::badbit);

	std::string s;
	char definitionsCstrs[10][20];

	//macros del compute shader
	D3D_SHADER_MACRO defines[11];
	defines[0].Name = "NUM_HEMICUBE_FACES";
	defines[1].Name = "NUM_THREADS_FOR_FINAL_STEP";
	defines[2].Name = "HEMICUBE_FACE_SIZE";
	defines[3].Name = "GROUP_DIMENSION_X";
	defines[4].Name = "FACES_PER_ROW";
	defines[5].Name = "HEMICUBE_FACE_SIZE_BITS";
	defines[6].Name = "HALF_FACE_SURFACE_BITS";
	defines[7].Name = "FACES_PER_ROW_BITS";
	defines[8].Name = "GROUP_DIMENSION_X_SECOND_SHADER";
	defines[9].Name = "DISPATCH_DIMENSION_X_BITS";

	try {
		ss << NUM_HEMICUBE_FACES;
		strncpy(definitionsCstrs[0], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[0].Definition = definitionsCstrs[0];

		ss << NUM_THREADS_FOR_FINAL_STEP;
		strncpy(definitionsCstrs[1], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[1].Definition = definitionsCstrs[1];

		ss << HEMICUBE_FACE_SIZE;
		strncpy(definitionsCstrs[2], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[2].Definition = definitionsCstrs[2];

		ss << GROUP_DIMENSION_X;
		strncpy(definitionsCstrs[3], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[3].Definition = definitionsCstrs[3];

		ss << (PARENT_HEMICUBES_TEXTURE_MAX_WIDTH/HEMICUBE_FACE_SIZE);
		strncpy(definitionsCstrs[4], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[4].Definition = definitionsCstrs[4];

		UINT bits = 1;
		UINT tmp = HEMICUBE_FACE_SIZE;
		while((tmp = tmp >> 1) != 1) ++bits;
		ss << bits;
		strncpy(definitionsCstrs[5], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[5].Definition = definitionsCstrs[5];

		bits = 1;
		tmp = (HEMICUBE_FACE_SIZE*HEMICUBE_FACE_SIZE)/2;
		while((tmp = tmp >> 1) != 1) ++bits;
		ss << bits;
		strncpy(definitionsCstrs[6], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[6].Definition = definitionsCstrs[6];

		bits = 1;
		tmp = (PARENT_HEMICUBES_TEXTURE_MAX_WIDTH/HEMICUBE_FACE_SIZE);
		while((tmp = tmp >> 1) != 1) ++bits;
		ss << bits;
		strncpy(definitionsCstrs[7], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[7].Definition = definitionsCstrs[7];

		ss << (DISPATCH_DIMENSION_X / 2);
		strncpy(definitionsCstrs[8], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[8].Definition = definitionsCstrs[8];

		bits = 1;
		tmp = DISPATCH_DIMENSION_X;
		while((tmp = tmp >> 1) != 1) ++bits;
		ss << bits;
		strncpy(definitionsCstrs[9], ss.str().c_str(), strlen(ss.str().c_str())+1);	ss.str(s);
		defines[9].Definition = definitionsCstrs[9];

		defines[10].Name = NULL;
		defines[10].Definition = NULL;
	}
	catch (std::stringstream::failure &) 
	{
		MiscErrorWarning(SS_ERROR);
		return E_FAIL;
	}

	//compilar los shaders
	if(FAILED( hr = m_giCalcStep1Shader.CompileComputeShader(GI_SHADER_FILE, string("IntegrationStepA"), defines) )) return hr;
	if(FAILED( hr = m_giCalcStep2Shader.CompileComputeShader(GI_SHADER_FILE, string("IntegrationStepB"), defines) )) return hr;
	if(FAILED( hr = m_addPassesShader.CompileComputeShader(GI_SHADER_FILE, string("AddPasses"), defines) )) return hr;

	m_giCalcStep1ComputeShader = m_giCalcStep1Shader.GetComputeShader();
	m_giCalcStep2ComputeShader = m_giCalcStep2Shader.GetComputeShader();
	m_addPassesComputeShader = m_addPassesShader.GetComputeShader();

	return S_OK;
}

}