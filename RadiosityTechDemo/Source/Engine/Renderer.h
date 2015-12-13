//------------------------------------------------------------------------------------------
// File: Renderer.h
//
// Esta clase define un renderizador capaz de dibujar toda una escena descripta por un 
// objeto de tipo Scene. Para ello hace uso, además, de objetos Skybox y ShadowMap.
// Puede mostrar el resultado final en el frame buffer y agregar un HUD con información
// relevante al mismo, o escribir la renderización en un render target previamente
// configurado.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef RENDERER_H
#define RENDERER_H

#include <ctime>
#include <sstream>

#include "Utility.h"

#include "Camera.h"
#include "Light.h"

#include "D3DDevicesManager.h"

#include "DirectionalShadowMap.h"
#include "OmniShadowMap.h"

#include "Scene.h"
#include "Skybox.h"
#include "Timer.h"

#include "D3D11DeviceStates.h"
#include "InputLayouts.h"

using std::wstringstream;
using std::endl;
using std::vector;
using std::wstring;

namespace DTFramework
{

class Renderer
{
public:
	Renderer(const D3DDevicesManager &d3d);
	~Renderer();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init(const LightType lightType, const UINT shadowMapSize=256, const UINT skyBoxLowResSize=64);

	//Renderiza y muestra el resultado en el frame buffer mediante Present
	HRESULT ProcessFrame(Scene &scene, const Camera &camera, Light &light, ID3D11ShaderResourceView *GIData);

	//Renderiza al render target previamente asignado
	HRESULT Render(Scene &scene, Light * const light, const D3DXVECTOR3 &cameraPosition, const D3DXMATRIX &view, const D3DXMATRIX &projection, 
	               ID3D11ShaderResourceView *GIData = NULL, const UINT rasterizerState=DEVICE_STATE_RASTER_SOLID_CULLBACK, const bool renderSky=true);

	//Renderiza geometría sin luz y skybox. Al render target previamente asignado
	HRESULT AuxiliarRenderDepthAndSkybox(Scene &scene, const Light &light, const D3DXVECTOR3 &cameraPosition, const D3DXMATRIX &view, const D3DXMATRIX &projection, 
	                                     const UINT rasterizerState=DEVICE_STATE_RASTER_SOLID_CULLBACK);

	//Cambia el tipo de luz del renderizador
	HRESULT ChangeLightType(const LightType lightType, const UINT shadowMapSize);

	void TurnOnOffHUD();
	void TurnOnOffGI();

private:
	void PrepareHUDInfo(const Camera &camera, const Light &light) const;
	HRESULT RenderSkyAndSun(const Light &light, const D3DXMATRIX &view, const D3DXMATRIX &projection, const bool lowRes=false);

private:
	const D3DDevicesManager &m_d3dManager;

	Timer m_timer;

	//input layouts
	InputLayouts m_inputLayouts;

	//device states
	D3D11DeviceStates *m_deviceStates;
	
	//shadow maps
	ShadowMap *m_shadowMap;
	LightType m_lightType;

	//skybox
	Skybox *m_skyBox;	

	//depthonly render
	CompiledShader m_depthOnlyEffect;
	ID3DX11EffectTechnique *m_depthOnlyTechnique;
	ID3DX11EffectMatrixVariable *m_depthOnlyWVP;

	//mostrar u ocultar el HUD e iluminación indirecta en el frame buffer
	bool m_hudEnabled;
	bool m_giEnabled;

	bool m_ready;
};

inline void Renderer::TurnOnOffHUD()
{
	m_hudEnabled = !m_hudEnabled;
}
inline void Renderer::TurnOnOffGI()
{
	m_giEnabled = !m_giEnabled;
}

}

#endif