//------------------------------------------------------------------------------------------
// File: CPURadiosity.h
//
// Esta clase implementa el algoritmo de radiosidad para su ejecución en una CPU.
// Computa el término de iluminación global para el cálculo del color
// final de cada pixel de la escena. Los datos tienen una densidad por vértice.
// Los cálculos aritméticos utilizan la biblioteca DirectXMath.
// 
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef CPU_RADIOSITY_H
#define CPU_RADIOSITY_H

#include "Radiosity.h"
#include <DirectXMath.h>

namespace DTFramework
{

class CPURadiosity : public Radiosity
{
public:
	CPURadiosity(const D3DDevicesManager &d3d, const bool exportHemicubes=false, const bool enableProfiling=false, const UINT verticesBakedPerDispatch=256, const UINT numBounces=2);
	virtual ~CPURadiosity();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();
	
	//calcula datos de iluminación indirecta dado un renderizador, una escena y una luz
	virtual HRESULT ComputeGIDataForScene(Renderer &renderer, Scene &scene, Light &light);

protected:
	void ComputeCPUAlgorithmConstants();
	HRESULT PrepareCPUAlgorithmBuffers(const Mesh &sceneMesh);

	virtual HRESULT IntegrateHemicubeRadiance(const UINT vertexId, const UINT verticesBaked, const UINT pass);

protected:
	DirectX::XMVECTOR *m_cpuGITempData;          //suma parcial (y total al finalizar)
	DirectX::XMVECTOR *m_currentPassCpuGIData;  //pasada actual
	
	ImmutableBuffer *m_lastPassBuffer;
	ImmutableBuffer *m_finalGIDataBuffer;

	//datos para el cálculo del algoritmo en CPU
	float m_uvFunction[HEMICUBE_FACE_SIZE];
	float m_weights[2][HEMICUBE_FACE_SIZE][HEMICUBE_FACE_SIZE];

	double m_integrationTimeMinusMemCpyTime;    //tiempo de integración en segundos (precisión en microsegundos) sin contar el tiempo de copiado de datos.
};

}

#endif