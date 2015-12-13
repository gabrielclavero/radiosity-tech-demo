//------------------------------------------------------------------------------------------
// File: CPURadiosity.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


#include "CPURadiosity.h"

namespace DTFramework
{

CPURadiosity::CPURadiosity(const D3DDevicesManager &d3d, const bool exportHemicubes, const bool enableProfiling, 
                           const UINT verticesBakedPerDispatch, const UINT numBounces)
: 
Radiosity(d3d, exportHemicubes, enableProfiling, verticesBakedPerDispatch, numBounces),
m_cpuGITempData(0), m_currentPassCpuGIData(0), m_lastPassBuffer(0), m_finalGIDataBuffer(0), m_integrationTimeMinusMemCpyTime(0)
{
	
}

CPURadiosity::~CPURadiosity()
{
	SAFE_DELETE(m_lastPassBuffer);
	SAFE_DELETE(m_finalGIDataBuffer);

	if(m_cpuGITempData) _aligned_free(m_cpuGITempData);
	if(m_currentPassCpuGIData) _aligned_free(m_currentPassCpuGIData);
}

HRESULT CPURadiosity::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CPURadiosity::Init");
		return E_FAIL;
	}

	HRESULT hr;

	//el hardware soporta la biblioteca DirectXMath?
	if(!DirectX::XMVerifyCPUSupport()) {
		MiscErrorWarning(NO_SSE_SUPPORT, NULL);
		return E_FAIL;
	}

	if(FAILED(hr = Radiosity::Init())) return hr;
	
	m_ready = true;

	return S_OK;
}

HRESULT CPURadiosity::ComputeGIDataForScene(Renderer &renderer, Scene &scene, Light &light)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CPURadiosity::ComputeGIDataForScene");
		return E_FAIL;
	}

	if(!scene.GetSceneMesh() || scene.GetSceneMesh()->GetTotalVertices() <= 0) {
		MiscErrorWarning(INVALID_PARAMETER, L"CPURadiosity::ComputeGIDataForScene");
		return E_INVALIDARG;
	}

	HRESULT hr;

	if(m_profiling) {
		m_hemicubeRenderingTime = 0;
		m_totalIntegrationTime = 0;
		m_totalAlgorithmTime = 0;
		m_integrationTimeMinusMemCpyTime = 0;

		m_timer2.UpdateForGPU();
	}

	ComputeCPUAlgorithmConstants();

	if(FAILED(hr = PrepareCPUAlgorithmBuffers(*(scene.GetSceneMesh())))) return hr;

	//preparar vector de vértices GI creados en base a los vértices del vertex buffer
	if(FAILED(hr = PrepareGIVerticesVector(*(scene.GetSceneMesh())))) return hr;

	//las pasadas, o iteraciones, representan el numero de veces que calculamos el rebote de la luz. Desde que sale de su origen.
	const bool showSky =  scene.ShowSky();
	const UINT numPasses = showSky ? PASSES : PASSES+1;

	for(UINT pass=0; pass < numPasses; ++pass) 
	{
		if(!showSky && pass == 0) continue;	//no consideramos a la luz del skybox para el GI si no hay cielo en la escena

		if(FAILED(hr = ProcessScene(renderer, scene, light, pass))) return hr;
		
		if(pass < numPasses-1) 
		{
			// copiamos a un buffer en memoria de video los datos del último pass (porque los necesitamos para la próxima renderización de hemicubos)
			const Mesh &mesh = *(scene.GetSceneMesh());

			SAFE_DELETE(m_lastPassBuffer);
				
			if((m_lastPassBuffer = new (std::nothrow) ImmutableBuffer(m_d3dManager, mesh.GetTotalVertices() * 16, mesh.GetTotalVertices(), 
			                                                          (void *) m_currentPassCpuGIData, DXGI_FORMAT_R32G32B32A32_FLOAT)) == NULL) 
			{
				MiscErrorWarning(BAD_ALLOC);
				return E_FAIL;
			}
			if(FAILED(hr = m_lastPassBuffer->Init())) return hr;

			m_lastPassGIDataSRV = m_lastPassBuffer->GetShaderResourceView();
		}
		else	
		{
			//en el ultimo pass guardar el finalsrv (que está en m_cpuGITempData)
			const Mesh &mesh = *(scene.GetSceneMesh());

			if((m_finalGIDataBuffer = new (std::nothrow) ImmutableBuffer(m_d3dManager, mesh.GetTotalVertices() * 16, mesh.GetTotalVertices(), 
			                                                             (void *) m_cpuGITempData, DXGI_FORMAT_R32G32B32A32_FLOAT)) == NULL)
			{
				MiscErrorWarning(BAD_ALLOC);
				return E_FAIL;
			}
			if(FAILED(hr = m_finalGIDataBuffer->Init())) return hr;

			m_finalGIDataSRV = m_finalGIDataBuffer->GetShaderResourceView();
		}
	}

	if(m_profiling) {
		m_timer2.UpdateForGPU();
		m_totalAlgorithmTime = m_timer2.GetTimeElapsed();

		m_outputFile << "RESULTS:" << endl << endl;
		m_outputFile << "Vertices in Scene:\t\t\t\t\t\t" << m_vertices.size() << endl;
		m_outputFile << "Hemicubes' Total Rendering Time:\t\t\t\t" << m_hemicubeRenderingTime << " seconds." << endl;
		m_outputFile << "Hemicubes' Total Integration Time:\t\t\t\t" << m_totalIntegrationTime << " seconds." << endl;
		m_outputFile << "Hemicubes' Total Integration Time Minus Memory Transfer:\t" << m_integrationTimeMinusMemCpyTime << " seconds." << endl;
		m_outputFile << "Radiosity Algorithm Total Time:\t\t\t\t\t" << m_totalAlgorithmTime << " seconds." << endl;
	}

	return S_OK;
}


HRESULT CPURadiosity::IntegrateHemicubeRadiance(const UINT vertexId, const UINT verticesBaked, const UINT pass)
{
	HRESULT hr;

	if(m_profiling)
		m_timer.Update();

	DirectX::XMVECTOR vertexIrradiance;

	//copiar el radiance map a un staging buffer para poder leerlo desde la CPU
	StagingTexture stagingTex(m_d3dManager, PARENT_HEMICUBES_TEXTURE_WIDTH, FACES_PER_COLUMN * HEMICUBE_FACE_SIZE, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(FAILED(hr = stagingTex.Init())) return hr;

	ID3D11Resource *pRes = NULL;
	m_hemiCubes->GetColorTexture()->GetResource(&pRes);

	m_d3dManager.CopyResource( stagingTex.GetTexture(), pRes );

	//map
	D3D11_MAPPED_SUBRESOURCE mapped;
	if(FAILED(hr = m_d3dManager.Map(stagingTex.GetTexture(), 0, D3D11_MAP_READ, 0, &mapped))) {
		SAFE_RELEASE(pRes);
		return hr;
	}

	const float *rawMapData = reinterpret_cast<float *>(mapped.pData);

	if(m_profiling) {
		m_timer.Update();
		m_totalIntegrationTime += m_timer.GetTimeElapsed();
	}


	if(m_exportHemicubes)
		ExportHemicubeFaces(rawMapData, vertexId, pass);

	//calcular irradiancia para los vertices a partir de sus radiancias sacadas del staging texture
	for(UINT i=0; i<verticesBaked; ++i)
	{
		vertexIrradiance = DirectX::XMVectorReplicate(0.0f);
		for(UINT j=0; j<NUM_HEMICUBE_FACES; ++j) 
		{
			for(UINT k=0; k<HEMICUBE_FACE_SIZE; ++k)	//coordenada v
			{
				if(j == 3 && k < HEMICUBE_FACE_SIZE / 2) continue;	//+y
				if(j == 4 && k >= HEMICUBE_FACE_SIZE / 2) break;	//-y

				for(UINT f=0; f<HEMICUBE_FACE_SIZE; ++f)	//coordenada u
				{
					if(j == 1 && f >= HEMICUBE_FACE_SIZE / 2) break;	//+x
					if(j == 2 && f < HEMICUBE_FACE_SIZE / 2) continue;	//-x

					const UINT faceNumber = i * NUM_HEMICUBE_FACES + j;
					const UINT faceRow = faceNumber / FACES_PER_ROW;
					const UINT faceCol = faceNumber % FACES_PER_ROW;

					const UINT index = ((faceCol * HEMICUBE_FACE_SIZE + f)  +  k * PARENT_HEMICUBES_TEXTURE_WIDTH + faceRow * HEMICUBE_FACE_SIZE * PARENT_HEMICUBES_TEXTURE_WIDTH) * 4;

					DirectX::XMVECTOR pixelRadiance = DirectX::XMVectorSet(rawMapData[index], rawMapData[index+1], rawMapData[index+2], 0.0f);

					const DirectX::XMVECTOR weight = DirectX::XMVectorReplicate(m_weights[j == 0 ? 0 : 1][j <= 2 ? k : f][j <= 2 ? f : k]);

					//al multiplicarlo por el delta form factor la radiancia se convierte en irradiancia
					pixelRadiance = DirectX::XMVectorMultiply(pixelRadiance, weight);
						
					vertexIrradiance = DirectX::XMVectorAdd(vertexIrradiance, pixelRadiance);
				}
			}
		}
		
		vertexIrradiance = DirectX::XMVectorMultiply(vertexIrradiance, DirectX::XMVectorReplicate(m_giCalcConstants.vertexWeight));

		//guardamos la vertexIrradiance en un bufer de cpu indexado por el vertexId y el object id
		m_currentPassCpuGIData[vertexId + i] = vertexIrradiance;

		//suma parcial (al final quedará la total aquí de manera que no necesitamos un método AddPassesCPU)
		m_cpuGITempData[vertexId + i] = DirectX::XMVectorAdd(m_cpuGITempData[vertexId + i], vertexIrradiance);
	}
	
	if(m_profiling) {
		m_timer.Update();

		m_integrationTimeMinusMemCpyTime += m_timer.GetTimeElapsed();
		m_totalIntegrationTime += m_timer.GetTimeElapsed();
	}

	m_d3dManager.Unmap(stagingTex.GetTexture(), 0);

	SAFE_RELEASE(pRes);

	return S_OK;
}

HRESULT CPURadiosity::PrepareCPUAlgorithmBuffers(const Mesh &sceneMesh)
{
	//borrar buffers anteriores
	if(m_cpuGITempData) _aligned_free(m_cpuGITempData);
	if(m_currentPassCpuGIData) _aligned_free(m_currentPassCpuGIData);

	m_cpuGITempData = NULL;
	m_currentPassCpuGIData = NULL;

	//reservar memoria con alineación de 16 bytes según lo requerido por la biblioteca DirectXMath
	m_cpuGITempData = (DirectX::XMVECTOR *) _aligned_malloc(sizeof(DirectX::XMVECTOR) * sceneMesh.GetTotalVertices(), 16);
	m_currentPassCpuGIData = (DirectX::XMVECTOR *) _aligned_malloc(sizeof(DirectX::XMVECTOR) * sceneMesh.GetTotalVertices(), 16);

	if(!m_cpuGITempData || !m_currentPassCpuGIData) {
		MiscErrorWarning(BAD_ALIGNED_ALLOC);
		return E_FAIL;
	}

	ZeroMemory(m_cpuGITempData, 16 * sceneMesh.GetTotalVertices());

	return S_OK;
}

void CPURadiosity::ComputeCPUAlgorithmConstants()
{
	if(m_profiling)
		m_timer.Update();

	//calcular datos que se usan repetidamente en el algoritmo

	//coordenadas u,v
	for(UINT i=0; i<HEMICUBE_FACE_SIZE; ++i)
		m_uvFunction[i] = (static_cast<float>(i) / static_cast<float>(HEMICUBE_FACE_SIZE-1)) * 2.0f - 1.0f;
	
	//delta form factors
	for(UINT i=0; i<HEMICUBE_FACE_SIZE; ++i) {  //coordenada v
		for(UINT j=0; j<HEMICUBE_FACE_SIZE; ++j) { //coordenada u
			float tmp = 1.0f + m_uvFunction[i] * m_uvFunction[i] + m_uvFunction[j] * m_uvFunction[j];
			tmp = tmp * tmp * (float) D3DX_PI;
			m_weights[0][i][j] = 1.0f / tmp;	//+z
			m_weights[1][i][j] = abs(m_uvFunction[j]) / tmp;	//las otras 4 caras
		}
	}

	if(m_profiling) {
		m_timer.Update();
		m_totalIntegrationTime += m_timer.GetTimeElapsed();
	}
}

}