//------------------------------------------------------------------------------------------
// File: DirectionalShadowMap.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "DirectionalShadowMap.h"

namespace DTFramework
{

DirectionalShadowMap::DirectionalShadowMap(const D3DDevicesManager &d3d, const UINT width, const UINT height) 
: ShadowMap(d3d, width, height), 
m_lightWVP(0)
{

}

DirectionalShadowMap::~DirectionalShadowMap()
{

}

HRESULT DirectionalShadowMap::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"DirectionalShadowMap::Init");
		return E_FAIL;
	}

	HRESULT hr;

	//shader
	if(FAILED(hr = PrepareShaderAndDeviceStates(DIR_SHADOW_EFFECT_FILE))) return hr;

	//renderable texture
	if((m_renderableTexture = new (std::nothrow) RenderableTexture(m_d3dManager)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED(hr = m_renderableTexture->Init(m_width, m_height, true))) return hr;

	//matrices
	m_lightWVP = m_shader.GetEffect()->GetVariableByName( "gLightWVP" )->AsMatrix();

	m_ready = true;

	return S_OK;
}

HRESULT DirectionalShadowMap::ComputeShadowMap(const Scene &scene, const Light &light)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"DirectionalShadowMap::ComputeShadowMap");
		return E_FAIL;
	}

	HRESULT hr;

	if(FAILED(hr = PrepareForRender())) return hr;

	m_d3dManager.RSSetState( m_deviceStates->GetRasterizerState( DEVICE_STATE_RASTER_SOLID_CULLBACK ) );
	
	//obtener la matriz WVP desde el punto de vista de la luz
	const D3DXMATRIX &viewProj = light.GetViewProjectionMatrix();

	if(FAILED(hr = m_lightWVP->SetMatrix( (float *) &viewProj ))) {
		DXGI_D3D_ErrorWarning(hr, L"DirectionalShadowMap::ComputeShadowMap-->SetMatrix");
		return hr;
	}

	//aplicar la technique luego de actualizar variables del shader
	if(FAILED( hr = m_d3dManager.ApplyEffectPass(m_technique->GetPassByIndex(0), 0) ) ) return hr;

	//vamos a dibujar toda la escena desde el punto de vista de la luz para obtener el depth map 
	//(no renderizamos al backbuffer solo nos interesa el depth buffer)
	hr = Render(scene);

	return hr;
}

#ifdef DEBUG_TEXTURES
HRESULT DirectionalShadowMap::DebugDrawShadow()
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"DirectionalShadowMap::DebugDrawShadow");
		return E_FAIL;
	}

	return m_renderableTexture->DebugDrawTexture(true);
}
#endif


}