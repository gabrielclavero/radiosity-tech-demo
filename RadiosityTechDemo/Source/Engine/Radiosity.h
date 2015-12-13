//------------------------------------------------------------------------------------------
// File: Radiosity.h
//
// Clase abstracta que define la interfaz y funcionalidades generales que necesita una 
// implementación del algoritmo de radiosidad. El algoritmo es implementado por las clases
// derivadas. Sólo la renderización de los hemicubos se define aquí.
// 
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef RADIOSITY_H
#define RADIOSITY_H

#define NOMINMAX

#include <malloc.h>
#include <algorithm>

#include "Utility.h"
#include "RenderableTexture.h"
#include "Scene.h"
#include "Renderer.h"
#include "D3D11DeviceStates.h"
#include "D3D11Resources.h"
#include "D3DDevicesManager.h"
#include "Timer.h"

using std::ofstream;
using std::max;
using std::min;

namespace DTFramework
{

struct GI_Calculation_Constants
{
	UINT vertexId;
	float vertexWeight;
	UINT verticesSkipped;
	UINT numVertices;
};

class Radiosity
{
public:
	Radiosity(const D3DDevicesManager &d3d, const bool exportHemicubes=false, const bool enableProfiling=false, 
	          const UINT verticesBakedPerDispatch=256, const UINT numBounces=2);
	virtual ~Radiosity();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();
	
	//calcula datos de iluminación indirecta dado un renderizador, una escena y una luz
	virtual HRESULT ComputeGIDataForScene(Renderer &renderer, Scene &scene, Light &light) = 0;

	//srv con valores de iluminación indirecta para cada vértice de la escena
	ID3D11ShaderResourceView *GetGIData() const;

	const UINT GetHemicubeFaceSize() const;

protected:
	void ComputeVertexWeight();

	HRESULT ProcessScene(Renderer &renderer, Scene &scene, Light &light, const UINT pass);
	HRESULT ProcessVertex(Renderer &renderer, Scene &scene, Light &light, const UINT pass, const UINT vertexId);

	virtual HRESULT IntegrateHemicubeRadiance(const UINT vertexId, const UINT verticesBaked, const UINT pass) = 0;

	HRESULT PrepareGIVerticesVector(const Mesh &mesh);

	void ExportHemicubeFaces(const float * const hemicubeData, const UINT vertexId, const UINT pass) const;

	static void GetFaceScissorRectangle(const UINT face, const UINT left, const UINT top, D3D11_RECT &scissorRect);
	static void VertexCameraMatrix(const GIVertex &vertex, const UINT face, D3DXMATRIX &viewMatrix);

protected:
	static const UINT NUM_HEMICUBE_FACES = 5;
	
	//ancho y alto de las caras del hemicubo
	static const UINT HEMICUBE_FACE_SIZE = 64;

	static const UINT PARENT_HEMICUBES_TEXTURE_MAX_WIDTH = 8192;

	//define cantidad de vértices a integrar por ejecución de IntegrateHemicubeRadiance
	const UINT VERTICES_BAKED_PER_DISPATCH;

	const UINT PASSES;

	//Todos los hemicubos están en un sólo RenderableTexture. Este es el ancho (en texels) del mismo. 
	//El máximo permitido es PARENT_HEMICUBES_TEXTURE_MAX_WIDTH y depende de VERTICES_BAKED_PER_DISPATCH
	const UINT PARENT_HEMICUBES_TEXTURE_WIDTH;		
	
	//Cantidad de caras de hemicubos que hay a lo ancho de la RenderableTexture principal. Se calcula automáticamente.
	const UINT FACES_PER_ROW;

	//Cantidad de caras de hemicubos que hay a lo alto de la RenderableTexture principal. Se calcula automáticamente.
	const UINT FACES_PER_COLUMN;



	const D3DDevicesManager &m_d3dManager;

	//constantes de GI
	GI_Calculation_Constants m_giCalcConstants;

	//renderable texture para los hemicubos
	RenderableTexture *m_hemiCubes;

	//depth stencil buffer para renderización de los hemicubos
	Texture2D_NOAA *m_depthStencilBuffer;


	//SRV de la ultima pasada. 
	ID3D11ShaderResourceView *m_lastPassGIDataSRV;

	//datos de iluminacion global finales. Es un valor de irradiancia en formato float4 para cada vértice de la escena
	ID3D11ShaderResourceView *m_finalGIDataSRV;

	//vertices de la escena en memoria de sistema
	vector<GIVertex> m_vertices;

	//Profiling
	const bool m_profiling;
	Timer m_timer;
	Timer m_timer2;
	double m_hemicubeRenderingTime;     //tiempo en segundos (precisión de microsegundos) que tardamos en renderizar todos los hemicubos (para todos los vértices)
	double m_totalIntegrationTime;     //tiempo en segundos (precisión de microsegundos) que tardamos en integrar todos los vértices
	double m_totalAlgorithmTime;      //tiempo en segundos (precisión de microsegundos) que tardamos en ejecutar todo el algoritmo
	ofstream m_outputFile;

	
	const bool m_exportHemicubes;
	
	bool m_ready;
};

inline ID3D11ShaderResourceView *Radiosity::GetGIData() const
{
	return m_finalGIDataSRV;
}

inline const UINT Radiosity::GetHemicubeFaceSize() const
{
	return HEMICUBE_FACE_SIZE;
}

}

#endif