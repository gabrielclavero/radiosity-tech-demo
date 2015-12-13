//------------------------------------------------------------------------------------------
// File: ShadowMap.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "ShadowMap.h"

namespace DTFramework
{

ShadowMap::ShadowMap(const D3DDevicesManager &d3d, const UINT width, const UINT height) 
: m_d3dManager(d3d), m_width(width), m_height(height), m_shader(d3d), m_renderableTexture(0), 
 m_inputLayouts(d3d), m_deviceStates(0), m_technique(0), m_ready(false)
{
	m_oldRenderTargets[0] = NULL;
	m_oldDepthStencilViews[0] = NULL;
	m_oldRasterizerState = NULL;
	m_oldDepthStencilState = NULL;
}

ShadowMap::~ShadowMap()
{
	SAFE_DELETE(m_renderableTexture);
	SAFE_DELETE(m_deviceStates);

	SAFE_RELEASE(m_oldRenderTargets[0]);
	SAFE_RELEASE(m_oldDepthStencilViews[0]);
	SAFE_RELEASE(m_oldDepthStencilState);
	SAFE_RELEASE(m_oldRasterizerState);
}

HRESULT ShadowMap::PrepareShaderAndDeviceStates(const wstring &shaderFile)
{
	HRESULT hr;

	//shader
	if(FAILED(hr = m_shader.LoadPrecompiledShader(shaderFile))) return hr;

	m_technique = m_shader.GetEffect()->GetTechniqueByName("BuildShadowMapTech");

	//input layouts. Creamos uno con solo posición
	if(FAILED(hr = m_inputLayouts.Init(INPUT_LAYOUT_POSITION_ONLY))) return hr;

	//device states
	if((m_deviceStates = new (std::nothrow) D3D11DeviceStates(m_d3dManager)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	hr = m_deviceStates->Init( DEVICE_STATE_RASTER_SOLID_CULLBACK | DEVICE_STATE_RASTER_SOLID_CULLFRONT | 
	                           DEVICE_STATE_DEPTHSTENCIL_ENABLED | DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR );

	return hr;
}

HRESULT ShadowMap::PrepareForRender()
{
	//Guardar los estados que venían seteados en el pipeline para restaurarlos luego de hacer el render shadowmap
	UINT numViewports = 1;
	m_d3dManager.RSGetViewports(&numViewports, m_oldViewports);
	m_d3dManager.RSGetState(&m_oldRasterizerState);
	m_d3dManager.OMGetRenderTargets(1, m_oldRenderTargets, m_oldDepthStencilViews);
	m_d3dManager.OMGetDepthStencilState(&m_oldDepthStencilState, &m_oldStencilRef);

	//setear input layout para el shadow render
	m_d3dManager.IASetInputLayout( m_inputLayouts.GetPositionOnlyInputLayout() );

	//setear render targets, viewports y depth stencil targets
	const float clearColor[4] = { 1.0, 1.0, 1.0, 1.0 };
	m_renderableTexture->Begin(clearColor);

	m_d3dManager.OMSetDepthStencilState( m_deviceStates->GetDepthStencilState( DEVICE_STATE_DEPTHSTENCIL_ENABLED ), 1 );

	return S_OK;
}

HRESULT ShadowMap::Render(const Scene &scene)
{
	HRESULT hr;

	//dibujar la escena 
	if(FAILED(hr = scene.DrawSceneMesh())) return hr;

	//restaurar estados del pipeline a lo que ya estaba
	m_d3dManager.OMSetRenderTargets(1, m_oldRenderTargets, m_oldDepthStencilViews[0]);
	m_d3dManager.RSSetViewports(1, m_oldViewports);
	m_d3dManager.RSSetState( m_oldRasterizerState );
	m_d3dManager.OMSetDepthStencilState( m_oldDepthStencilState, m_oldStencilRef );

	SAFE_RELEASE(m_oldRenderTargets[0]);	//la llamada a OMGetRenderTargets incrementa el reference count del ID3D11RenderTargetView * y ID3D11DepthStencilView * bindeados
	SAFE_RELEASE(m_oldDepthStencilViews[0]);
	SAFE_RELEASE(m_oldDepthStencilState);
	SAFE_RELEASE(m_oldRasterizerState);

	return S_OK;
}

}