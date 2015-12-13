//------------------------------------------------------------------------------------------
// File: GPURadiosity.h
//
// Esta clase implementa el algoritmo de radiosidad para su ejecución en una GPU.
// Computa el término de iluminación global para el cálculo del color
// final de cada pixel de la escena. Los datos tienen una densidad por vértice.
// La integración se realiza en los compute shaders implementados en el archivo
// Shaders/HemicubesIntegration.hlsl
// 
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef GPU_RADIOSITY_H
#define GPU_RADIOSITY_H

#include "Radiosity.h"
#include "CompiledShader.h"
#include "Profiler.h"

namespace DTFramework
{

class GPURadiosity : public Radiosity
{
public:
	GPURadiosity(const D3DDevicesManager &d3d, const bool exportHemicubes=false, const bool enableProfiling=false, 
	             const UINT verticesBakedPerDispatch=256, const UINT verticesBakedPerDispatch2=1, const UINT numBounces=2);
	virtual ~GPURadiosity();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();
	
	//calcula datos de iluminación indirecta dado un renderizador, una escena y una luz
	virtual HRESULT ComputeGIDataForScene(Renderer &renderer, Scene &scene, Light &light);

protected:
	HRESULT CompileComputeShaders();
	HRESULT PrepareGPUAlgorithmBuffers(const Mesh &sceneMesh);

	virtual HRESULT IntegrateHemicubeRadiance(const UINT vertexId, const UINT verticesBaked, const UINT pass);
	HRESULT AddPasses(const UINT pass);

protected:
	//número de hilos por grupo para ejecutar el shader de suma de pasadas
	static const UINT NUM_THREADS_FOR_FINAL_STEP = 1024;

	//dimensión X del primer dispatch. Es decir cantidad de grupos necesarios para procesar un vértice
	static const UINT DISPATCH_DIMENSION_X = 64;

	//dimensión X de los grupos del primer dispatch (total de pixeles de un hemicubo / (DISPATCH_DIMENSION_X*2)
	static const UINT GROUP_DIMENSION_X = 96;

	//define la cantidad de ejecuciones del primer shader de integración antes de ejecutar el segundo
	const UINT VERTICES_BAKED_PER_DISPATCH_2;
	
	//sumas parciales de la integración realizadas por el primer dispatch
	WritableBuffer *m_partialIntegrationBuffer;

	//valores de irradiancia para cada vértice. En cada iteración se suman las irradiancias conseguidas en ella
	WritableBuffer *m_GITempData[2];

	//irradiancias de cada vértice para la iteración actual y la última completada
	WritableBuffer *m_currentAndLastPassGIData[2];
	
	//compute shaders
	CompiledShader m_giCalcStep1Shader;
	CompiledShader m_giCalcStep2Shader;
	CompiledShader m_addPassesShader;
	ID3D11ComputeShader *m_giCalcStep1ComputeShader;
	ID3D11ComputeShader *m_giCalcStep2ComputeShader;
	ID3D11ComputeShader *m_addPassesComputeShader;


	UINT m_verticesReadyForStepBMultiplier;
	UINT m_verticesBakedInCurrentPass;
	UINT m_totalVertices;

	//constantes de GI
	ConstantBuffer *m_giCalcConstantsBuffer;
	ConstantBuffer *m_UVConstantsBuffer;

	//profiling
	Profiler m_profiler;
	double m_addPassesTime;				//tiempo en segundos (precisión en microsegundos) que tardamos en sumar las iteraciones
};

}

#endif