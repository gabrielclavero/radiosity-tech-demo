//------------------------------------------------------------------------------------------
// File: Renderer.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Renderer.h"

namespace DTFramework
{

Renderer::Renderer(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_timer(d3d), m_inputLayouts(d3d), m_deviceStates(0), m_shadowMap(0), m_lightType(DIRECTIONAL_LIGHT), m_skyBox(0),
m_depthOnlyEffect(d3d), m_depthOnlyTechnique(0), m_depthOnlyWVP(0), m_hudEnabled(true), m_giEnabled(true), m_ready(false)
{

}

Renderer::~Renderer()
{
	SAFE_DELETE(m_deviceStates);
	SAFE_DELETE(m_skyBox);
	SAFE_DELETE(m_shadowMap);
}

HRESULT Renderer::ChangeLightType(const LightType lightType, const UINT shadowMapSize)
{
	HRESULT hr;

	if(shadowMapSize == 0) {
		MiscErrorWarning(INVALID_PARAMETER, L"Renderer::ChangeLightType");
		return E_FAIL;
	}

	SAFE_DELETE(m_shadowMap);

	m_lightType = lightType;
	
	//shadow maps
	switch(lightType) 
	{
		case POINT_LIGHT:
			if((m_shadowMap = new (std::nothrow) OmniShadowMap( m_d3dManager, shadowMapSize, shadowMapSize )) == NULL) {	//crear el objeto shadow
				MiscErrorWarning(BAD_ALLOC);
				return E_FAIL;
			}

			if( FAILED(hr = m_shadowMap->Init( )) ) return hr;	//inicializarlo

			break;

		case DIRECTIONAL_LIGHT:
			if((m_shadowMap = new (std::nothrow) DirectionalShadowMap( m_d3dManager, shadowMapSize, shadowMapSize )) == NULL) {
				MiscErrorWarning(BAD_ALLOC);
				return E_FAIL;
			}

			if( FAILED(hr = m_shadowMap->Init( ))) return hr;

			break;

		default:
			return E_FAIL;
	}

	return S_OK;
}

HRESULT Renderer::Init(const LightType lightType, const UINT shadowMapSize, const UINT skyBoxLowResSize)
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Renderer::Init");
		return E_FAIL;
	}

	if(shadowMapSize == 0 || skyBoxLowResSize == 0) {
		MiscErrorWarning(INVALID_PARAMETER, L"Renderer::Init");
		return E_FAIL;
	}

	HRESULT hr;
	
	if(FAILED(hr = ChangeLightType(lightType, shadowMapSize))) return hr;

	//skybox
	if((m_skyBox = new (std::nothrow) Skybox(m_d3dManager)) == NULL) {
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}
	if(FAILED(hr = m_skyBox->Init())) return hr;
	if(FAILED(hr = m_skyBox->CreateLowResTexture(skyBoxLowResSize, skyBoxLowResSize))) return  hr;

	//depth only effect
	if(FAILED(hr = m_depthOnlyEffect.LoadPrecompiledShader(DEPTH_ONLY_EFFECT_FILE))) return hr;
	ID3DX11Effect *tmp = m_depthOnlyEffect.GetEffect();
	m_depthOnlyTechnique = tmp->GetTechniqueByName("DepthOnlyRenderTechnique");
	m_depthOnlyWVP = tmp->GetVariableByName("gWVP")->AsMatrix();

	//input layouts. Creamos dos. Uno con vertex de 4 elementos (posicion, normal, tex coord y tangente) y otro con solo posición
	if(FAILED(hr = m_inputLayouts.Init(INPUT_LAYOUT_STANDARD | INPUT_LAYOUT_POSITION_ONLY))) return hr;

	//Primitive topology. Nunca debería cambiar
	m_d3dManager.IASetPrimitiveTopology(  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	//device states. Estos son los device states que usaremos en las renderizaciones normales y auxiliares
	if((m_deviceStates = new (std::nothrow) D3D11DeviceStates(m_d3dManager)) == NULL) {
		MiscErrorWarning(INVALID_PARAMETER, L"Renderer::Init");
		return E_FAIL;
	}
	if(FAILED(hr = m_deviceStates->Init(DEVICE_STATE_RASTER_SOLID_CULLBACK | DEVICE_STATE_RASTER_SOLID_CULLFRONT | DEVICE_STATE_RASTER_SOLID_CULLNONE_SCISSOR | 
	                                    DEVICE_STATE_BLEND_TRANSPARENT | DEVICE_STATE_DEPTHSTENCIL_ENABLED | DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR ))) return hr;

	//timer para el HUD
	m_timer.Start();

	m_ready = true;

	return hr;
}

HRESULT Renderer::ProcessFrame(Scene &scene, const Camera &camera, Light &light, ID3D11ShaderResourceView *GIData)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Renderer::ProcessFrame");
		return E_FAIL;
	}

	HRESULT hr;

	//es necesario que la luz light sea del mismo tipo que el que tiene el renderer
	if(light.GetType() != m_lightType) {
		MiscErrorWarning(INVALID_PARAMETER, L"Renderer::ProcessFrame");
		return E_FAIL;
	}

	m_timer.Update();
	
	// limpiar el back buffer
	m_d3dManager.ClearBackBuffer(0.0f, 0.0f, 0.0f, 1.0f);
	
	// limpiar el depth stencil buffer
	m_d3dManager.ClearDepthStencilBuffer();

	//renderizar al backbuffer
	m_d3dManager.ResetRenderingToBackBuffer();

	//renderizar
	if(FAILED(hr = Render(scene, &light, camera.GetCameraPosition(), camera.GetViewMatrix(), camera.GetProjectionMatrix(), m_giEnabled ? GIData : NULL) )) return hr;

	//info de fps, posición y orientación de la cámara. Información sobre la luz activa y controles
	if(m_hudEnabled)
		PrepareHUDInfo(camera, light);

	#if defined(DEBUG_TEXTURES)
		m_d3dManager.IASetInputLayout( m_inputLayouts.GetStandardInputLayout() );
		m_shadowMap->DebugDrawShadow();
	#endif

	//mostrar la escena y el HUD. Este último solo si estaba activo
	if(FAILED(hr = m_d3dManager.Present())) DXGI_D3D_ErrorWarning(hr, L"Present");

	return hr;
}

//------------------------------------------------------------------------------------------
// light == NULL => se renderiza sin iluminación directa (y por lo tanto sin cielo, si lo hubiera en la escena)
// GIData == NULL => se renderiza sin iluminación indirecta.
//
// Render target debe ser previamente configurado antes de llamar a esta función.
//------------------------------------------------------------------------------------------
HRESULT Renderer::Render(Scene &scene, Light * const light, const D3DXVECTOR3 &cameraPosition, const D3DXMATRIX &view, const D3DXMATRIX &projection, 
                         ID3D11ShaderResourceView *GIData, const UINT rasterizerState, const bool renderSky)
{
	_ASSERT(m_ready);

	_ASSERT(!light || light->GetType() == m_lightType);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Renderer::Render");
		return E_FAIL;
	}

	if(light && light->GetType() != m_lightType) {
		MiscErrorWarning(INVALID_PARAMETER, L"Renderer::Render");
		return E_INVALIDARG;
	}

	HRESULT hr=S_OK;

	const D3DXMATRIX wvpMatrix = view * projection;		//world es identidad

	//1. si especificamos una luz y aún no se actualizó, calculamos las sombras
	if(light && light->ShouldUpdate()) {
		if(FAILED(hr = m_shadowMap->ComputeShadowMap(scene, *light) )) return hr;
	}

	//2. pipeline states
	m_d3dManager.OMSetBlendState();
	m_d3dManager.OMSetDepthStencilState( m_deviceStates->GetDepthStencilState( DEVICE_STATE_DEPTHSTENCIL_ENABLED ), 1 );
	m_d3dManager.RSSetState( m_deviceStates->GetRasterizerState( rasterizerState ) );
	m_d3dManager.IASetInputLayout( m_inputLayouts.GetStandardInputLayout() );
	m_d3dManager.IASetPrimitiveTopology(  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	//3. dibujar la escena
	if(light && light->ShouldUpdate()) {
		if(FAILED(hr = scene.Render(&cameraPosition, &(light->GetProperties()), 1, m_shadowMap->GetShadowMap(), 
		                            light->GetType() != POINT_LIGHT ? &(light->GetViewProjectionMatrix()) : NULL, GIData, &wvpMatrix )))
		{
			return hr;
		}

		light->OnOffUpdateDone();
	} else if(light) {
		if(FAILED(hr = scene.Render(&cameraPosition, NULL, 1, NULL, NULL, GIData, &wvpMatrix))) return hr;
	} else {
		//si light es NULL quiere decir que no debemos renderizar con luces por lo tanto pasamos 0 active lights aquí
		if(FAILED(hr = scene.Render(&cameraPosition, NULL, 0, NULL, NULL, GIData, &wvpMatrix))) return hr;
	}

	//4. renderizar el cielo LUEGO de renderizar la escena es una técnica de optimización. El cielo se renderiza con profundidad 1.0f
	//por lo tanto no se escribe el mismo pixel dos veces. Lo que sucedería con algunos si renderizaramos en el orden inverso.
	if(light && scene.ShowSky() && renderSky) {
		if(FAILED(hr = RenderSkyAndSun(*light, view, projection))) return hr;
	}

	return hr;
}

HRESULT Renderer::AuxiliarRenderDepthAndSkybox(Scene &scene, const Light &light, const D3DXVECTOR3 &cameraPosition, const D3DXMATRIX &view, 
                                               const D3DXMATRIX &projection, const UINT rasterizerState)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Renderer::AuxiliarRenderDepthAndSkybox");
		return E_FAIL;
	}

	HRESULT hr;

	//pipeline states
	m_d3dManager.RSSetState( m_deviceStates->GetRasterizerState( rasterizerState ));
	m_d3dManager.OMSetBlendState();
	m_d3dManager.OMSetDepthStencilState( m_deviceStates->GetDepthStencilState( DEVICE_STATE_DEPTHSTENCIL_ENABLED ), 1 );
	m_d3dManager.IASetPrimitiveTopology(  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_d3dManager.IASetInputLayout(m_inputLayouts.GetPositionOnlyInputLayout());

	//1. depthonly
	D3DXMATRIX viewProjection = view * projection;
	if(FAILED(hr = m_depthOnlyWVP->SetMatrix((float *) &viewProjection))) { 
		DXGI_D3D_ErrorWarning(hr, L"SetMatrix");
		return hr; 
	}

	if(FAILED(hr = m_d3dManager.ApplyEffectPass(m_depthOnlyTechnique->GetPassByIndex(0), 0) )) return hr; 
	if(FAILED(hr = scene.DrawSceneMesh())) return hr;

	//2. skybox
	if(FAILED(hr = RenderSkyAndSun(light, view, projection, true))) return hr;

	return S_OK;
}

void Renderer::PrepareHUDInfo(const Camera &camera, const Light &light) const
{
	wstringstream tmp;

	wstring lightType;
	switch( light.GetType() ) 
	{
		case DIRECTIONAL_LIGHT:
			lightType = L"DIRECTIONAL";
			break;
		case POINT_LIGHT:
			lightType = L"POINT";
			break;
	}
	tmp << m_timer.GetFramesPerSecond() << endl << "Milisec per frame: " << (1.0f/m_timer.GetFramesPerSecond()) * 1000 << endl
										<< "Camera Position: " << camera.GetCameraPosition().x << " " << camera.GetCameraPosition().y << " " << camera.GetCameraPosition().z
										<< endl << "Camera Direction: " << camera.GetCameraRotation().x << " " << camera.GetCameraRotation().y << " " << camera.GetCameraRotation().z
										<< endl << "Is light dynamic?: " << light.IsDynamic()
										<< endl << "Shadow Bias: " << light.GetShadowMapBias()
										<< endl << "Light Type: " << lightType
										<< endl << "Is light on? " << light.IsOn();

	m_d3dManager.DrawString(tmp.str().c_str(), 14.0f, 5.0f, 5.0f, 0xffffffff, FW1_RESTORESTATE);

	wstring instructions(L"CONTROLS:\n\nW A S D: Up, Left, Down, Right\nG: Turn On/Off Direct Lighting\nF: Turn On/Off Dynamic Light\nZ: Turn On/Off GI\nX: Turn On/Off HUD");

	const D3D11_VIEWPORT &vp = m_d3dManager.GetViewPort();

	m_d3dManager.DrawString(instructions.c_str(), 18.0f, 5.0f, vp.Height - 155.0f, 0xffffffff, FW1_RESTORESTATE);
}

HRESULT Renderer::RenderSkyAndSun(const Light &light, const D3DXMATRIX &view, const D3DXMATRIX &projection, const bool lowRes)
{
	_ASSERT(m_ready);

	D3DXVECTOR3 sunDirection = light.GetDirection() - light.GetPosition();
	D3DXVec3Normalize(&sunDirection, &sunDirection);

	D3DXVECTOR3 bias = D3DXVECTOR3(0.84f, 0.84f, 0.74f);

	return m_skyBox->Render(view, projection, sunDirection, bias, lowRes);
}

}