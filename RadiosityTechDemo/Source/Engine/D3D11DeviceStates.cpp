//------------------------------------------------------------------------------------------
// File: D3D11DeviceStates.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "D3D11DeviceStates.h"

namespace DTFramework
{

//inicializar los estados que flags indique
HRESULT D3D11DeviceStates::Init(const UINT flags)
{
	HRESULT hr = S_OK;

	if(FAILED(hr = PrepareDepthStencilStates(flags)))
		return hr;

	if(FAILED(hr = PrepareRasterizerStates(flags)))
		return hr;

	hr = PrepareBlendStates(flags);

	return hr;
}

HRESULT D3D11DeviceStates::PrepareDepthStencilStates(const UINT flags)
{
	HRESULT hr = S_OK;
	
	D3D11_DEPTH_STENCIL_DESC dsDesc;

	if(flags & DEVICE_STATE_DEPTHSTENCIL_ENABLED)
	{
		// depth test 
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

		// stencil test 
		dsDesc.StencilEnable = false;	//no stencil siempre
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace = dsDesc.FrontFace;

		// depth stencil state
		if( FAILED( hr = m_d3dManager.CreateDepthStencilState(&dsDesc, &m_depthStencilStateEnabled) )) return hr;
	}
	if(flags & DEVICE_STATE_DEPTHSTENCIL_DISABLED)
	{
		// depth test 
		dsDesc.DepthEnable = false;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

		// stencil test 
		dsDesc.StencilEnable = false;	//no stencil siempre
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace = dsDesc.FrontFace;

		// depth stencil state
		if( FAILED( hr = m_d3dManager.CreateDepthStencilState(&dsDesc, &m_depthStencilStateDisabled) )) return hr;
	}
	if(flags & DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ZERO)
	{
		// depth test 
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		// stencil test 
		dsDesc.StencilEnable = false;	//no stencil siempre
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace = dsDesc.FrontFace;

		// depth stencil state
		if( FAILED( hr = m_d3dManager.CreateDepthStencilState(&dsDesc, &m_depthStencilStateEnabledEqual) )) return hr;
	}

	if(flags & DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ALL)
	{
		// depth test 
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		// stencil test 
		dsDesc.StencilEnable = false;	//no stencil siempre
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace = dsDesc.FrontFace;

		// depth stencil state
		if( FAILED( hr = m_d3dManager.CreateDepthStencilState(&dsDesc, &m_depthStencilStateEnabledEqualMask) )) return hr;
	}
	
	return hr;
}

HRESULT D3D11DeviceStates::PrepareRasterizerStates(const UINT flags)
{
	HRESULT hr = S_OK;

	//crear un estado rasterizer que le dice a la etapa de rasterizado como comportarse
	D3D11_RASTERIZER_DESC rastDesc;

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLBACK) 
	{
		rastDesc.FillMode = D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_BACK;		//no dibujar back facing faces
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthBias = 0;
		rastDesc.DepthBiasClamp = 0.0f;
		rastDesc.SlopeScaledDepthBias = 0.0f;
		rastDesc.DepthClipEnable = true;
		rastDesc.ScissorEnable = false;
		rastDesc.MultisampleEnable = true;
		rastDesc.AntialiasedLineEnable = false;

		if(FAILED(hr = m_d3dManager.CreateRasterizerState(&rastDesc, &m_rasterStateSolidBack))) return hr;
	}

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLFRONT) 
	{
		rastDesc.FillMode = D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_FRONT;		//no dibujar front facing faces
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthBias = 0;
		rastDesc.DepthBiasClamp = 0.0f;
		rastDesc.SlopeScaledDepthBias = 0.0f;
		rastDesc.DepthClipEnable = true;
		rastDesc.ScissorEnable = false;
		rastDesc.MultisampleEnable = true;
		rastDesc.AntialiasedLineEnable = false;

		if(FAILED(hr = m_d3dManager.CreateRasterizerState(&rastDesc, &m_rasterStateSolidFront))) return hr;
	}

	if(flags & DEVICE_STATE_RASTER_WIRE) 
	{
		rastDesc.FillMode = D3D11_FILL_WIREFRAME;
		rastDesc.CullMode = D3D11_CULL_FRONT;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthBias = 0;
		rastDesc.DepthBiasClamp = 0.0f;
		rastDesc.SlopeScaledDepthBias = 0.0f;
		rastDesc.DepthClipEnable = true;
		rastDesc.ScissorEnable = false;
		rastDesc.MultisampleEnable = true;
		rastDesc.AntialiasedLineEnable = false;

		if(FAILED(hr = m_d3dManager.CreateRasterizerState(&rastDesc, &m_rasterStateWire))) return hr;
	}

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLNONE) 
	{
		rastDesc.FillMode = D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_NONE;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthBias = 0;
		rastDesc.DepthBiasClamp = 0.0f;
		rastDesc.SlopeScaledDepthBias = 0.0f;
		rastDesc.DepthClipEnable = true;
		rastDesc.ScissorEnable = false;
		rastDesc.MultisampleEnable = true;
		rastDesc.AntialiasedLineEnable = false;

		if(FAILED(hr = m_d3dManager.CreateRasterizerState(&rastDesc, &m_rasterStateCullNone))) return hr;
	}

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLNONE_SCISSOR) 
	{
		rastDesc.FillMode = D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_NONE;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthBias = 0;
		rastDesc.DepthBiasClamp = 0.0f;
		rastDesc.SlopeScaledDepthBias = 0.0f;
		rastDesc.DepthClipEnable = true;
		rastDesc.ScissorEnable = true;
		rastDesc.MultisampleEnable = true;
		rastDesc.AntialiasedLineEnable = false;

		if(FAILED(hr = m_d3dManager.CreateRasterizerState(&rastDesc, &m_rasterStateCullNoneScissor))) return hr;
	}

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR) 
	{
		rastDesc.FillMode = D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_BACK;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthBias = 0;
		rastDesc.DepthBiasClamp = 0.0f;
		rastDesc.SlopeScaledDepthBias = 0.0f;
		rastDesc.DepthClipEnable = true;
		rastDesc.ScissorEnable = true;
		rastDesc.MultisampleEnable = true;
		rastDesc.AntialiasedLineEnable = false;

		if(FAILED(hr = m_d3dManager.CreateRasterizerState(&rastDesc, &m_rasterStateCullBackScissor))) return hr;
	}

	return hr;
}

HRESULT D3D11DeviceStates::PrepareBlendStates(const UINT flags)
{
	HRESULT hr = S_OK;

	D3D11_BLEND_DESC blendDesc = {0};

	if(flags & DEVICE_STATE_BLEND_TRANSPARENT)
	{
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		
		hr = m_d3dManager.CreateBlendState(&blendDesc, &m_transparentBlendState);
	}

	return hr;
}

ID3D11DepthStencilState *D3D11DeviceStates::GetDepthStencilState(const UINT flags) const
{
	if(flags & DEVICE_STATE_DEPTHSTENCIL_ENABLED)
		return m_depthStencilStateEnabled;
	
	if(flags & DEVICE_STATE_DEPTHSTENCIL_DISABLED)
		return m_depthStencilStateDisabled;

	if(flags & DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ZERO)
		return m_depthStencilStateEnabledEqual;

	if(flags & DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ALL)
		return m_depthStencilStateEnabledEqualMask;

	return NULL;
}

ID3D11BlendState *D3D11DeviceStates::GetBlendState(const UINT flags) const
{
	if(flags & DEVICE_STATE_BLEND_ALPHA)
		return m_alphaBlendState;

	if(flags & DEVICE_STATE_BLEND_TRANSPARENT)
		return m_transparentBlendState;

	return NULL;
}

ID3D11RasterizerState *D3D11DeviceStates::GetRasterizerState(const UINT flags) const
{
	if(flags & DEVICE_STATE_RASTER_SOLID_CULLBACK)
		return m_rasterStateSolidBack;

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLFRONT)
		return m_rasterStateSolidFront;

	if(flags & DEVICE_STATE_RASTER_WIRE)
		return m_rasterStateWire;

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLNONE)
		return m_rasterStateCullNone;

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLNONE_SCISSOR)
		return m_rasterStateCullNoneScissor;

	if(flags & DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR)
		return m_rasterStateCullBackScissor;

	return NULL;
}

}