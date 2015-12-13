//------------------------------------------------------------------------------------------
// File: D3D11DeviceStates.h
//
// Esta clase facilita la creación de estados para diferentes etapas de la graphics pipeline
// expuesta por directx.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef D3D11DEVICE_STATES_H
#define D3D11DEVICE_STATES_H

#include "Utility.h"
#include "D3DDevicesManager.h"

namespace DTFramework
{

namespace
{
	//blend state flags
	const UINT DEVICE_STATE_BLEND_TRANSPARENT = 1;
	const UINT DEVICE_STATE_BLEND_ALPHA = 2;	//reservado. No implementado

	//raster state flags
	const UINT DEVICE_STATE_RASTER_SOLID_CULLBACK = 4;
	const UINT DEVICE_STATE_RASTER_SOLID_CULLFRONT = 8;
	const UINT DEVICE_STATE_RASTER_WIRE = 16;
	const UINT DEVICE_STATE_RASTER_SOLID_CULLNONE = 32;
	const UINT DEVICE_STATE_RASTER_SOLID_CULLNONE_SCISSOR = 64;
	const UINT DEVICE_STATE_RASTER_SOLID_CULLBACK_SCISSOR = 1024;

	//depth stencil flags
	const UINT DEVICE_STATE_DEPTHSTENCIL_ENABLED = 128;
	const UINT DEVICE_STATE_DEPTHSTENCIL_DISABLED = 256;
	const UINT DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ZERO = 512;	//lo usa el skybox
	const UINT DEVICE_STATE_DEPTHSTENCIL_ENABLED_EQUAL_MASK_ALL = 2048;	//no se usa en ningún lado por ahora
}

class D3D11DeviceStates
{
public:
	D3D11DeviceStates(const D3DDevicesManager &d3d);
	~D3D11DeviceStates();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init(const UINT flags=0);

	ID3D11DepthStencilState *GetDepthStencilState(const UINT flags) const;
	ID3D11BlendState *GetBlendState(const UINT flags) const;
	ID3D11RasterizerState *GetRasterizerState(const UINT flags) const;

private:
	HRESULT PrepareDepthStencilStates(const UINT flags);
	HRESULT PrepareRasterizerStates(const UINT flags);
	HRESULT PrepareBlendStates(const UINT flags);

private:
	const D3DDevicesManager &m_d3dManager;

	ID3D11DepthStencilState *m_depthStencilStateEnabled;
	ID3D11DepthStencilState *m_depthStencilStateDisabled;
	ID3D11DepthStencilState *m_depthStencilStateEnabledEqual;
	ID3D11DepthStencilState *m_depthStencilStateEnabledEqualMask;

	ID3D11RasterizerState *m_rasterStateSolidBack;
	ID3D11RasterizerState *m_rasterStateSolidFront;
	ID3D11RasterizerState *m_rasterStateWire;
	ID3D11RasterizerState *m_rasterStateCullNone;
	ID3D11RasterizerState *m_rasterStateCullNoneScissor;
	ID3D11RasterizerState *m_rasterStateCullBackScissor;
	
	ID3D11BlendState *m_transparentBlendState;
	ID3D11BlendState *m_alphaBlendState;
};

inline D3D11DeviceStates::D3D11DeviceStates(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_depthStencilStateEnabled(0), m_depthStencilStateEnabledEqual(0), 
m_depthStencilStateEnabledEqualMask(0), m_depthStencilStateDisabled(0),
m_rasterStateSolidBack(0), m_rasterStateSolidFront(0), m_rasterStateWire(0),
m_rasterStateCullNone(0), m_rasterStateCullNoneScissor(0), m_rasterStateCullBackScissor(0),
m_transparentBlendState(0), m_alphaBlendState(0)
{

}

inline D3D11DeviceStates::~D3D11DeviceStates()
{
	SAFE_RELEASE(m_depthStencilStateEnabled);
	SAFE_RELEASE(m_depthStencilStateDisabled);
	SAFE_RELEASE(m_depthStencilStateEnabledEqual);
	SAFE_RELEASE(m_depthStencilStateEnabledEqualMask);

	SAFE_RELEASE(m_rasterStateSolidBack);
	SAFE_RELEASE(m_rasterStateSolidFront);
	SAFE_RELEASE(m_rasterStateWire);
	SAFE_RELEASE(m_rasterStateCullNone);
	SAFE_RELEASE(m_rasterStateCullNoneScissor);
	SAFE_RELEASE(m_rasterStateCullBackScissor);

	SAFE_RELEASE(m_transparentBlendState);
	SAFE_RELEASE(m_alphaBlendState);
}

}

#endif