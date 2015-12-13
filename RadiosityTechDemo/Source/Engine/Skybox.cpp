//------------------------------------------------------------------------------------------
// File: Skybox.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Skybox.h"

namespace DTFramework
{

//definición de static member variables
const UINT Skybox::NUM_INDICES = 36;
const UINT Skybox::NUM_INDICES_SKYBOX = 6;


Skybox::Skybox(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_skyTextureEffect(d3d), m_skyBoxEffect(d3d), m_inputLayouts(d3d), m_skyTexture(0), m_skyTextureLowRes(0),
m_biasVariable(0), m_sunDirectionVariable(0), m_viewMatrixVariable(0), m_projMatrixVariable(0),
m_technique(0), m_vertexBuffer(0), m_indexBuffer(0), m_skyTextureVariable(0), m_skyBoxTechnique(0), m_skyBoxVertexBuffer(0), m_skyBoxIndexBuffer(0),
m_deviceStates(0), m_ready(false)
{

}

Skybox::~Skybox()
{
	SAFE_DELETE(m_skyTexture);
	SAFE_DELETE(m_skyTextureLowRes);

	SAFE_DELETE(m_deviceStates);

	SAFE_DELETE(m_skyBoxVertexBuffer);
	SAFE_DELETE(m_skyBoxIndexBuffer);
	SAFE_DELETE(m_vertexBuffer);
	SAFE_DELETE(m_indexBuffer);
}

HRESULT Skybox::CreateLowResTexture(const UINT width, const UINT height)
{
	SAFE_DELETE(m_skyTextureLowRes);

	if((m_skyTextureLowRes = new (std::nothrow) RenderableTexture(m_d3dManager)) == NULL) { 
		MiscErrorWarning(BAD_ALLOC); 
		return E_FAIL;
	}

	return m_skyTextureLowRes->Init(width, height, false, 1.0f);
}

HRESULT Skybox::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Skybox::Init");
		return E_FAIL;
	}

	HRESULT hr;

	// obtener viewport principal
	D3D11_VIEWPORT mainViewPort = m_d3dManager.GetViewPort();

	try 
	{
		m_skyTexture = new RenderableTexture(m_d3dManager);

		if(FAILED(hr = m_skyTexture->Init(static_cast<UINT>(mainViewPort.Width), static_cast<UINT>(mainViewPort.Height), false, 1.0f))) return hr;

		if(!FAILED(hr = m_skyTextureEffect.LoadPrecompiledShader(SKY_TEXTURE_EFFECT_FILE))) 
		{
			ID3DX11Effect *tmp = m_skyTextureEffect.GetEffect();

			m_biasVariable = tmp->GetVariableByName("gBias")->AsVector();
			m_sunDirectionVariable = tmp->GetVariableByName("gSunDir")->AsVector();

			m_viewMatrixVariable = tmp->GetVariableByName("gView")->AsMatrix();
			m_projMatrixVariable = tmp->GetVariableByName("gProjection")->AsMatrix();

			m_technique = tmp->GetTechniqueByName("SkyTextureTechnique");
		} 
		else
			return hr;

		//crear e inicializar el vertex e index buffer del sky texture
		D3DXVECTOR3 vertices[] =
		{
			D3DXVECTOR3(-1, 1, 1),
			D3DXVECTOR3(1, 1, 1),
			D3DXVECTOR3(1, -1, 1),
			D3DXVECTOR3(-1, -1, 1),
			D3DXVECTOR3(1, 1, -1),
			D3DXVECTOR3(-1, 1, -1),
			D3DXVECTOR3(-1, -1, -1),
			D3DXVECTOR3(1, -1,- 1),
		};

		m_vertexBuffer = new VertexBuffer(m_d3dManager, sizeof(vertices), vertices);

		if(FAILED( hr = m_vertexBuffer->Init() )) return hr;

		USHORT indices[] =
		{
			0, 1, 2, 2, 3, 0,
			1, 4, 7, 7, 2, 1,
			4, 5, 6, 6, 7, 4,
			5, 0, 3, 3, 6, 5,
			5, 4, 1, 1, 0, 5,
			3, 2, 7, 7, 6, 3 
		};

		m_indexBuffer = new IndexBuffer(m_d3dManager, sizeof(indices), indices);

		if(FAILED( hr = m_indexBuffer->Init())) return hr;

		//creación de estados d3d necesarios para el skybox.
		m_deviceStates = new D3D11DeviceStates(m_d3dManager);
		if(FAILED( hr = m_deviceStates->Init(DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ZERO ) )) return hr;

		if(!FAILED(hr = m_skyBoxEffect.LoadPrecompiledShader(SKY_BOX_EFFECT_FILE))) 
		{
			ID3DX11Effect *tmp = m_skyBoxEffect.GetEffect();
			m_skyTextureVariable = tmp->GetVariableByName("SkyTexture")->AsShaderResource();
			m_skyBoxTechnique = tmp->GetTechniqueByName("SkyBoxTechnique");
		} 
		else
			return hr;

		//input layouts
		if(FAILED( hr = m_inputLayouts.Init(INPUT_LAYOUT_POSITION_ONLY | INPUT_LAYOUT_POSITION_TEX) )) return hr;

		//vetex e index buffers del sky box
		SimpleVertex qv[4];

		D3DXVECTOR4 pos[] = 
		{
			D3DXVECTOR4(1.0f,  1.0f, 0,1),
			D3DXVECTOR4(1.0f, -1.0f, 0, 1),
			D3DXVECTOR4(-1.0f, -1.0f, 0, 1),
			D3DXVECTOR4(-1.0f, 1.0f, 0, 1)
		};

		D3DXVECTOR2 tex[] = 
		{
			D3DXVECTOR2(1, 0), 
			D3DXVECTOR2(1, 1),
			D3DXVECTOR2(0, 1),
			D3DXVECTOR2(0, 0)
		};

		for(int i = 0; i < 4; ++i)
		{
			qv[i].position = pos[i];
			qv[i].texCoord = tex[i];
		}
	
		USHORT quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

		m_skyBoxVertexBuffer = new VertexBuffer(m_d3dManager, sizeof(qv), qv);
	
		if(FAILED( hr = m_skyBoxVertexBuffer->Init())) return hr;

		m_skyBoxIndexBuffer = new IndexBuffer(m_d3dManager, sizeof(quadIndices), quadIndices);

		if(FAILED(hr = m_skyBoxIndexBuffer->Init())) return hr;

		m_ready = true;
	}
	catch(std::bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		hr = E_FAIL;
	}

	return hr;
}


HRESULT Skybox::Render(const D3DXMATRIX &view, const D3DXMATRIX &projection, const D3DXVECTOR3 &sunDir, const D3DXVECTOR3 bias, const bool useLowResTexture)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Skybox::Render");
		return E_FAIL;
	}

	if(useLowResTexture && !m_skyTextureLowRes) {
		MiscErrorWarning(INVALID_PARAMETER, L"Skybox::Render");
		return E_INVALIDARG;
	}

	HRESULT hr;

	//0. Guardar los estados que venían seteados en el pipeline para restaurarlos luego de hacer el render skybox
	UINT numViewports = 1;
	D3D11_VIEWPORT oldViewports[1];
	m_d3dManager.RSGetViewports(&numViewports, oldViewports);

	ID3D11RasterizerState *oldRasterizerState = nullptr;
	m_d3dManager.RSGetState(&oldRasterizerState);

	ID3D11RenderTargetView *renderTargets[1];
	ID3D11DepthStencilView *depthStencilViews[1];
	m_d3dManager.OMGetRenderTargets(1, renderTargets, depthStencilViews);

	ID3D11DepthStencilState *oldDepthStencilState = nullptr;
	UINT oldStencilRef;
	m_d3dManager.OMGetDepthStencilState(&oldDepthStencilState, &oldStencilRef);

	// 1. Dibujamos el sky a una textura

	//pipeline states
	m_d3dManager.RSSetState( m_deviceStates->GetRasterizerState( DEVICE_STATE_RASTER_SOLID_CULLNONE ));
	m_d3dManager.OMSetDepthStencilState( m_deviceStates->GetDepthStencilState( DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ZERO ), 0);
	m_d3dManager.IASetInputLayout(m_inputLayouts.GetPositionOnlyInputLayout());

	//vertex buffer
	UINT stride = sizeof(D3DXVECTOR3);
	UINT offset = 0;
	ID3D11Buffer *tmpBuffer = m_vertexBuffer->GetBuffer();
	m_d3dManager.IASetVertexBuffers(0, 1, &tmpBuffer, &stride, &offset);

	//index buffer
	m_d3dManager.IASetIndexBuffer(m_indexBuffer->GetBuffer(), DXGI_FORMAT_R16_UINT, 0);

	//primitive topology
	m_d3dManager.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//actualizar valores de variables en el shader
	HRESULT hr1, hr2, hr3, hr4;
	hr1 = m_viewMatrixVariable->SetMatrix((float *) &view);
	hr2 = m_projMatrixVariable->SetMatrix((float *) &projection);
	hr3 = m_sunDirectionVariable->SetFloatVector((float *) &sunDir);
	hr4 = m_biasVariable->SetFloatVector((float *) &bias);
		
	if(FAILED(hr1) || FAILED(hr2) || FAILED(hr3) || FAILED(hr4)) {
		DXGI_D3D_ErrorWarning(E_FAIL, L"Skybox::Render --> Set Shader Variables");
		SAFE_RELEASE(renderTargets[0]);
		SAFE_RELEASE(depthStencilViews[0]);
		SAFE_RELEASE(oldDepthStencilState);
		SAFE_RELEASE(oldRasterizerState);
		return E_FAIL;
	}

	if(FAILED( hr = m_d3dManager.ApplyEffectPass(m_technique->GetPassByIndex(0), 0) )) {
		SAFE_RELEASE(renderTargets[0]);
		SAFE_RELEASE(depthStencilViews[0]);
		SAFE_RELEASE(oldDepthStencilState);
		SAFE_RELEASE(oldRasterizerState);
		return hr;
	}

	//textura que usaremos (si es GI usamos una más chica, pues primero renderizamos el sky texture y 
	//luego lo usamos para el sky box pero las caras del hemicubo son sólo de 64x64)
	if(useLowResTexture)
		m_skyTextureLowRes->Begin();
	else
		m_skyTexture->Begin();

	//dibujar
	m_d3dManager.DrawIndexed(NUM_INDICES, 0, 0);

	if(useLowResTexture)
		m_skyTextureLowRes->End();
	else
		m_skyTexture->End();


	// 2. Ponemos la textura del sky a un quad que ocupa la mitad superior del render target que estaba seteado al llamar a esta función render

	//pipeline states
	m_d3dManager.OMSetRenderTargets(1, renderTargets, depthStencilViews[0]);
	float oldMinDepth = oldViewports[0].MinDepth;
	oldViewports[0].MinDepth = 1.0f;	//el cielo aparece detrás de todo. Tiene la mayor profundidad
	m_d3dManager.RSSetViewports(1, oldViewports);
	m_d3dManager.RSSetState( oldRasterizerState );
	m_d3dManager.IASetInputLayout(m_inputLayouts.GetPositionTexInputLayout());

	//vertex buffer
	stride = sizeof(SimpleVertex);
	offset = 0;
	tmpBuffer = m_skyBoxVertexBuffer->GetBuffer();
	m_d3dManager.IASetVertexBuffers(0, 1, &tmpBuffer, &stride, &offset);

	//index buffer
	m_d3dManager.IASetIndexBuffer(m_skyBoxIndexBuffer->GetBuffer(), DXGI_FORMAT_R16_UINT, 0);
	
	//pasar textura
	if(FAILED( hr = m_skyTextureVariable->SetResource( useLowResTexture ? m_skyTextureLowRes->GetColorTexture() : m_skyTexture->GetColorTexture() ))) {
		DXGI_D3D_ErrorWarning(hr, L"Skybox::Render --> SetResource");
		SAFE_RELEASE(renderTargets[0]);
		SAFE_RELEASE(depthStencilViews[0]);
		SAFE_RELEASE(oldDepthStencilState);
		SAFE_RELEASE(oldRasterizerState);
		return hr;
	}
	if(FAILED( hr = m_d3dManager.ApplyEffectPass(m_skyBoxTechnique->GetPassByIndex(0), 0) )) {
		SAFE_RELEASE(renderTargets[0]);
		SAFE_RELEASE(depthStencilViews[0]);
		SAFE_RELEASE(oldDepthStencilState);
		SAFE_RELEASE(oldRasterizerState);
		return hr;
	}

	//dibujar
	m_d3dManager.DrawIndexed(NUM_INDICES_SKYBOX, 0, 0);

	//restaurar lo que falta
	m_d3dManager.OMSetDepthStencilState( oldDepthStencilState, oldStencilRef );
	oldViewports[0].MinDepth = oldMinDepth;
	m_d3dManager.RSSetViewports(1, oldViewports);

	//clean up
	SAFE_RELEASE(renderTargets[0]);	//la llamada a OMGetRenderTargets incrementa el reference count del ID3D11RenderTargetView * y ID3D11DepthStencilView * bindeados
	SAFE_RELEASE(depthStencilViews[0]);
	SAFE_RELEASE(oldDepthStencilState);
	SAFE_RELEASE(oldRasterizerState);

	return hr;
}

}