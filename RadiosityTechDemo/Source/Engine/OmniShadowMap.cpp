//------------------------------------------------------------------------------------------
// File: OmniShadowMap.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "OmniShadowMap.h"

namespace DTFramework
{

OmniShadowMap::OmniShadowMap(const D3DDevicesManager &d3d, const UINT width, const UINT height)
: ShadowMap(d3d, width, height), 
m_viewMatrixVariable(0), m_worldMatrixVariable(0), m_projMatrixVariable(0), m_lightPos(0), m_lightRange(0)
{

}

OmniShadowMap::~OmniShadowMap()
{

}

HRESULT OmniShadowMap::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"OmniShadowMap::Init");
		return E_FAIL;
	}

	HRESULT hr;

	//shader
	if(FAILED(hr = PrepareShaderAndDeviceStates(OMNI_SHADOW_EFFECT_FILE))) return hr;

	//renderable texture
	if((m_renderableTexture = new (std::nothrow) RenderableTexture(m_d3dManager)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED(hr = m_renderableTexture->Init(m_width, m_height, false, 0.0f, DXGI_FORMAT_R32_FLOAT, false, 6, true))) return hr;
	
	//generar view matrices que miren hacia los 6 ejes +-x +-y +-z
	const float height = 1.5f;
	D3DXVECTOR3 eye = D3DXVECTOR3( 0.0f, height, 0.0f );
	D3DXVECTOR3 at;
	D3DXVECTOR3 up;

	at = D3DXVECTOR3( 1.0f, height, 0.0f );
	up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &m_cubeMapViewAdjust[0], &eye, &at, &up );
	at = D3DXVECTOR3( -1.0f, height, 0.0f );
	up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &m_cubeMapViewAdjust[1], &eye, &at, &up );
	at = D3DXVECTOR3( 0.0f, height + 1.0f, 0.0f );
	up = D3DXVECTOR3( 0.0f, 0.0f, -1.0f );
	D3DXMatrixLookAtLH( &m_cubeMapViewAdjust[2], &eye, &at, &up );
	at = D3DXVECTOR3( 0.0f, height - 1.0f, 0.0f );
	up = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
	D3DXMatrixLookAtLH( &m_cubeMapViewAdjust[3], &eye, &at, &up );
	at = D3DXVECTOR3( 0.0f, height, 1.0f );
	up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &m_cubeMapViewAdjust[4], &eye, &at, &up );
	at = D3DXVECTOR3( 0.0f, height, -1.0f );
	up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &m_cubeMapViewAdjust[5], &eye, &at, &up );

	m_viewMatrixVariable = m_shader.GetEffect()->GetVariableByName( "gView" )->AsMatrix();
	m_worldMatrixVariable = m_shader.GetEffect()->GetVariableByName( "gWorld" )->AsMatrix();
	m_projMatrixVariable = m_shader.GetEffect()->GetVariableByName( "gProj" )->AsMatrix();

	m_lightRange = m_shader.GetEffect()->GetVariableByName( "gLightRange" )->AsScalar();
	m_lightPos = m_shader.GetEffect()->GetVariableByName( "gLightPosW" )->AsVector();

	m_ready = true;

	return S_OK;
}

HRESULT OmniShadowMap::ComputeShadowMap(const Scene &scene, const Light &light)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"OmniShadowMap::ComputeShadowMap");
		return E_FAIL;
	}

	HRESULT hr;

	//perspective projection matrix de 90 grados
	D3DXMatrixPerspectiveFovLH( &m_proj, static_cast<float>(D3DX_PI) * 0.5f, 1.0f, light.GetZNear(), light.GetZFar());

	//valores que dependen de la luz
	if(FAILED(hr = m_projMatrixVariable->SetMatrix( (float *) &m_proj))) {
		DXGI_D3D_ErrorWarning(hr, L"OmniShadowMap::ComputeShadowMap-->SetMatrix");
		return hr;
	}
	if(FAILED(hr = m_lightRange->SetFloat( light.GetRange() ))) {
		DXGI_D3D_ErrorWarning(hr, L"OmniShadowMap::ComputeShadowMap-->SetFloat");
		return hr;
	}
	if(FAILED(hr = m_lightPos->SetFloatVector( (float *) &(light.GetPosition()) ))) {
		DXGI_D3D_ErrorWarning(hr, L"OmniShadowMap::ComputeShadowMap-->SetFloatVector");
		return hr;
	}

	if(FAILED(hr = PrepareForRender())) return hr;

	//usamos otro estado de rasterizado porque no vamos a calcular la profundidad de las front faces sino de las back faces para evitar artifacts
	m_d3dManager.RSSetState( m_deviceStates->GetRasterizerState( DEVICE_STATE_RASTER_SOLID_CULLFRONT ) );

	const D3DXVECTOR3 lightPos = light.GetPosition();

	if(FAILED(hr = m_lightPos->SetFloatVector( (float *) &lightPos ))) {
		DXGI_D3D_ErrorWarning(hr, L"OmniShadowMap::ComputeShadowMap --> SetFloatVector");
		return hr;
	}

	// matriz de transformación con posición igual a la posición de la luz
	D3DXMATRIX viewAlign;
	D3DXMatrixIdentity( &viewAlign );
	viewAlign._41 = -lightPos.x;
	viewAlign._42 = -lightPos.y;
	viewAlign._43 = -lightPos.z;

	//combinar dicha matriz con las 6 direcciones posibles de visión para obtener las 6 view matrices finales
	D3DXMATRIX viewMatrices[6];
	for( int view = 0; view < 6; ++view )
		D3DXMatrixMultiply( &viewMatrices[view], &viewAlign, &m_cubeMapViewAdjust[view] );

	if(FAILED(hr = m_viewMatrixVariable->SetMatrixArray( ( float* )viewMatrices, 0, 6 ) )) {
		DXGI_D3D_ErrorWarning(hr, L"OmniShadowMap::ComputeShadowMap --> SetMatrixArray");
		return hr;
	}
	
	D3DXMATRIX world;
	D3DXMatrixIdentity(&world);
	if(FAILED(hr = m_worldMatrixVariable->SetMatrix((float *) &( world ) ))) {
		DXGI_D3D_ErrorWarning(hr, L"OmniShadowMap::ComputeShadowMap-->SetMatrix");
		return hr;
	}

	if(FAILED( hr = m_d3dManager.ApplyEffectPass(m_technique->GetPassByIndex(0), 0) ) ) return  hr;

	
	//dibujamos toda la escena desde el punto de vista de la luz hacia los 6 ejes para obtener el depth map cúbico ya que una luz omni esparce luz en todas direcciones
	hr = Render(scene);

	return hr;
}

#ifdef DEBUG_TEXTURES
HRESULT OmniShadowMap::DebugDrawShadow()
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"OmniShadowMap::DebugDrawShadow");
		return E_FAIL;
	}

	return m_renderableTexture->DebugDrawTexture(false, true);
}
#endif

}