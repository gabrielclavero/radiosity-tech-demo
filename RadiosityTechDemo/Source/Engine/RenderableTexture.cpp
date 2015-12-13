//------------------------------------------------------------------------------------------
// File: RenderableTexture.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "RenderableTexture.h"

namespace DTFramework
{

#ifndef DEBUG_TEXTURES
RenderableTexture::RenderableTexture(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_colorTexture(0), m_depthTexture(0), m_width(0), m_height(0), m_viewPort(0), m_ready(false)
{
	
}
#endif

#ifdef DEBUG_TEXTURES
RenderableTexture::RenderableTexture(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_colorTexture(0), m_depthTexture(0), m_width(0), m_height(0), m_viewPort(0), m_ready(false),
 shader(d3d), m_NDCQuadVB(0), drawTextureTechnique(0), drawTextureVariable(0), drawTextureCubeVariable(0),
singleDepthVariable(0)
{

}
#endif

RenderableTexture::~RenderableTexture()
{
	SAFE_DELETE(m_colorTexture);
	SAFE_DELETE(m_depthTexture);
	SAFE_DELETE(m_viewPort);

	#ifdef DEBUG_TEXTURES
		SAFE_DELETE(m_NDCQuadVB);
	#endif
}

HRESULT RenderableTexture::Init(const UINT width, const UINT height, const bool depthOnly, const float minDepth, const DXGI_FORMAT format, 
                                const bool renderTargetOnly, const UINT arraySize, const bool isCube)
{
	_ASSERT(!m_ready);

	_ASSERT(!depthOnly || !renderTargetOnly);

	if(m_ready || (depthOnly && renderTargetOnly)) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"RenderableTexture::Init");
		return E_FAIL;
	}

	HRESULT hr;

	if(width <= 0 || height <= 0) {
		MiscErrorWarning(INVALID_PARAMETER, L"RenderableTexture::Init");
		return E_FAIL;
	}

	m_width = width;
	m_height = height;

	//view port
	if(!renderTargetOnly) 
	{
		if((m_viewPort = new (std::nothrow) D3D11_VIEWPORT) == NULL) { MiscErrorWarning(BAD_ALLOC); return E_FAIL; }
		m_viewPort->TopLeftX = 0;
		m_viewPort->TopLeftY = 0;
		m_viewPort->Width = static_cast<float>(width);
		m_viewPort->Height = static_cast<float>(height);
		m_viewPort->MinDepth = minDepth;
		m_viewPort->MaxDepth = 1.0f;
	}
	
	if(!depthOnly) {
		if(FAILED( hr = InitColorTexture(format, arraySize, isCube) ) ) return hr;
	}

	if(!renderTargetOnly) {
		if(FAILED(hr = InitDepthTexture(arraySize, isCube))) return hr;
	}

	//debug
	#ifdef DEBUG_TEXTURES
		if(FAILED(hr = shader.LoadPrecompiledShader(RENDER_TEXTURE_DEBUG_EFFECT_FILE))) return hr;
		drawTextureVariable = shader.GetEffect()->GetVariableByName("gMap")->AsShaderResource();
		drawTextureCubeVariable = shader.GetEffect()->GetVariableByName("gEnvMap")->AsShaderResource();
		singleDepthVariable = shader.GetEffect()->GetVariableByName("gSingleDepth")->AsScalar();
		drawTextureTechnique = shader.GetEffect()->GetTechniqueByName("DrawMapTech");
		if(FAILED(hr = BuildNDCQuad())) return hr;
	#endif

	m_ready = true;

	return hr;
}

HRESULT RenderableTexture::InitColorTexture(const DXGI_FORMAT format, const UINT arraySize, const bool isCube)
{
	HRESULT hr;

	if((m_colorTexture = new (std::nothrow) 
	                          Texture2D_NOAA(m_d3dManager, m_width, m_height, 1, arraySize, format, D3D11_USAGE_DEFAULT, 
	                                         D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0)) == NULL)
	{
		MiscErrorWarning(BAD_ALLOC); 
		return E_FAIL;
	}

	if(FAILED( hr = m_colorTexture->Init()) ) return hr;

	return hr;
}

HRESULT RenderableTexture::InitDepthTexture(const UINT arraySize, const bool isCube)
{
	HRESULT hr;

	if((m_depthTexture = new (std::nothrow) 
	                          Texture2D_NOAA(m_d3dManager, m_width, m_height, 1, arraySize, isCube ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_R32_TYPELESS, D3D11_USAGE_DEFAULT, 
	                                         D3D11_BIND_DEPTH_STENCIL | (isCube ? 0 : D3D11_BIND_SHADER_RESOURCE), 0, isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0)) == NULL)
	{
		MiscErrorWarning(BAD_ALLOC); 
		return E_FAIL;
	}

	if(FAILED( hr = m_depthTexture->Init() )) return hr;

	return hr;
}

void RenderableTexture::Begin(const float * const color)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"RenderableTexture::Begin");
		return;
	}

	ID3D11RenderTargetView *renderTargets[1] = { m_colorTexture ? m_colorTexture->GetRenderTargetView() : NULL};
	m_d3dManager.OMSetRenderTargets(1, renderTargets, m_depthTexture ? m_depthTexture->GetDepthStencilView() : NULL);

	m_d3dManager.RSSetViewports(1, m_viewPort);

	m_d3dManager.ClearDepthStencilView(m_depthTexture ? m_depthTexture->GetDepthStencilView() : NULL, D3D11_CLEAR_DEPTH, 1.0f, 0);

	if(m_colorTexture) 
	{
		if(color)
			m_d3dManager.ClearRenderTargetView(m_colorTexture ? m_colorTexture->GetRenderTargetView() : NULL, color);
		else {
			float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
			m_d3dManager.ClearRenderTargetView(m_colorTexture ? m_colorTexture->GetRenderTargetView() : NULL, black);
		}
	}
}

void RenderableTexture::End()
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"RenderableTexture::End");
		return;
	}
}

#ifdef DEBUG_TEXTURES
HRESULT RenderableTexture::BuildNDCQuad()
{
	D3DXVECTOR3 pos[] = 
	{
		D3DXVECTOR3(0.0f, -1.0f, 0.0f),
		D3DXVECTOR3(0.0f,  0.0f, 0.0f),
		D3DXVECTOR3(1.0f,  0.0f, 0.0f),

		D3DXVECTOR3(0.0f, -1.0f, 0.0f),
		D3DXVECTOR3(1.0f,  0.0f, 0.0f),
		D3DXVECTOR3(1.0f, -1.0f, 0.0f)
	};

	D3DXVECTOR2 tex[] = 
	{
		D3DXVECTOR2(0.0f, 1.0f),
		D3DXVECTOR2(0.0f, 0.0f),
		D3DXVECTOR2(1.0f, 0.0f),

		D3DXVECTOR2(0.0f, 1.0f),
		D3DXVECTOR2(1.0f, 0.0f),
		D3DXVECTOR2(1.0f, 1.0f)
	};

	Vertex qv[6];

	for(int i = 0; i < 6; ++i)
	{
		qv[i].position = pos[i];
		qv[i].texcoord = tex[i];
	}

	if((m_NDCQuadVB = new (std::nothrow) VertexBuffer(m_d3dManager, sizeof(Vertex) * 6, qv)) == NULL) { MiscErrorWarning(BAD_ALLOC); return E_FAIL; }

	return m_NDCQuadVB->Init();
}

HRESULT RenderableTexture::DebugDrawTexture(const bool renderDepthTexture, const bool renderCube)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"RenderableTexture::DebugDrawTexture");
		return E_FAIL;
	}

	HRESULT hr;

	//debug
	const UINT stride = sizeof(Vertex);
	const UINT offset = 0;
	ID3D11Buffer *buf = m_NDCQuadVB->GetBuffer();
	m_d3dManager.IASetVertexBuffers(0, 1, &buf, &stride, &offset);
	
	if(renderCube) {
		if(FAILED(hr = drawTextureCubeVariable->SetResource(renderDepthTexture ? (m_depthTexture ? m_depthTexture->GetShaderResourceView() : NULL) 
																			   : (m_colorTexture ? m_colorTexture->GetShaderResourceView() : NULL))))
		{
			DXGI_D3D_ErrorWarning(hr, L"RenderableTexture::DebugDrawTexture --> SetResource");
			return hr;
		}
	}
	else {
		if(FAILED(hr = drawTextureVariable->SetResource(renderDepthTexture ? (m_depthTexture ? m_depthTexture->GetShaderResourceView() : NULL)
																		   : (m_colorTexture ? m_colorTexture->GetShaderResourceView() : NULL))))
		{
			DXGI_D3D_ErrorWarning(hr, L"RenderableTexture::DebugDrawTexture --> SetResource");
			return hr;
		}
	}

	if(FAILED(hr = singleDepthVariable->SetBool(renderCube ? false : true))) {
		DXGI_D3D_ErrorWarning(hr, L"RenderableTexture::DebugDrawTexture --> SetBool");
		return hr;
	}

	if(FAILED(hr = m_d3dManager.ApplyEffectPass(drawTextureTechnique->GetPassByIndex(0), 0))) return hr;
	m_d3dManager.Draw(6, 0);

	return hr;
}
#endif

}