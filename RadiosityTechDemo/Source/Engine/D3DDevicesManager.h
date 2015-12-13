//------------------------------------------------------------------------------------------
// File: D3DDevicesManager.h
//
// Esta clase es responsable de inicializar Direct3D, de coordinar sus estados y de proveer
// una interfaz de acceso a algunas de las funcionalidades del device y device context.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef D3DDEVICES_MANAGER_H
#define D3DDEVICES_MANAGER_H

#include "Utility.h"

#include "FW1FontWrapper.h"
#include "d3dx11effect.h"

#include <d3d10.h>
#include <d3dx10.h>

namespace DTFramework
{

class D3DDevicesManager
{
public:
	D3DDevicesManager();
	~D3DDevicesManager();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init(const DXGI_MODE_DESC &mode, const bool windowed, const bool vsync, const bool aa, 
	             const HWND window, const UINT totalBackBuffers);

	HRESULT Present() const;
	
	void ResetRenderingToBackBuffer() const;

	void ClearBackBuffer(float r, float g, float b, float a) const;
	void ClearDepthStencilBuffer(const float depth=1.0f) const;

	const D3D11_VIEWPORT &GetViewPort() const;
	
	void PrepareForExit();

	void DrawString(const WCHAR *text, FLOAT FontSize, FLOAT X, FLOAT Y, UINT32 Color, UINT Flags) const;

	//funcionalidad de ID3D11Device y ID3D11DeviceContext expuesta con estas funciones
	void IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY Topology ) const;
	void IASetInputLayout(ID3D11InputLayout *pInputLayout) const;
	void IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets) const;
	void IASetIndexBuffer(ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset) const;

	void OMSetBlendState(ID3D11BlendState *blendState=NULL, const FLOAT *BlendFactor=NULL, UINT SampleMask=0xffffffff) const;
	void OMSetDepthStencilState(ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef) const;
	void OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView) const;
	void OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView) const;
	void OMGetDepthStencilState(ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef) const;


	void RSSetState(ID3D11RasterizerState *pRasterizerState) const;
	void RSGetState(ID3D11RasterizerState **ppRasterizerState) const;
	void RSGetScissorRects(UINT *pNumRects, D3D11_RECT *pRects) const;
	void RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT *pViewports) const;
	void RSGetViewports(UINT *pNumViewports, D3D11_VIEWPORT *pViewports) const;
	void RSSetScissorRects(UINT NumRects, const D3D11_RECT *pRects) const;

	void ClearDepthStencilView(ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) const;
	void ClearRenderTargetView(ID3D11RenderTargetView *pRenderTargetView, const FLOAT *ColorRGBA) const;

	void GenerateMips(ID3D11ShaderResourceView *pShaderResourceView) const;
	void Draw(UINT VertexCount, UINT StartVertexLocation) const;
	void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) const;

	HRESULT ApplyEffectPass(ID3DX11EffectPass *effectPass, const UINT flags) const;


	HRESULT CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D11DepthStencilState **ppDepthStencilState) const;
	HRESULT CreateRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc, ID3D11RasterizerState **ppRasterizerState) const;
	HRESULT CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc, ID3D11BlendState **ppBlendState) const;
	HRESULT CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements,
	                          const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, 
	                          ID3D11InputLayout **ppInputLayout) const;

	HRESULT CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer) const;
	HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D) const;

	HRESULT CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView) const;
	HRESULT CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView) const;
	HRESULT CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,  ID3D11RenderTargetView **ppRTView) const;
	HRESULT CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView) const;

	HRESULT CreateEffectFromMemory(void *pData, SIZE_T DataLength, UINT FXFlags, ID3DX11Effect **ppEffect) const;

	HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader) const;
	HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader) const;
	HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength,  ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppComputeShader) const;


	HRESULT CreateShaderResourceViewFromFileD3D11(LPCTSTR pSrcFile, D3DX11_IMAGE_LOAD_INFO *pLoadInfo, ID3DX11ThreadPump *pPump,
	                                              ID3D11ShaderResourceView **ppShaderResourceView, HRESULT *pHResult) const;

	HRESULT CreateTextureFromFileD3D11(LPCTSTR pSrcFile, D3DX11_IMAGE_LOAD_INFO *pLoadInfo,  ID3DX11ThreadPump *pPump,
	                                   ID3D11Resource **ppTexture,  HRESULT *pHResult) const;

	void CopyResource(ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource) const;
	HRESULT Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource) const;
	void Unmap(ID3D11Resource *pResource, UINT Subresource) const;
	
	void VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const;
	void PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const;
	void GSSetShader(ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const;
	void DSSetShader(ID3D11DomainShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const;
	void HSSetShader(ID3D11HullShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const;
	void CSSetShader(ID3D11ComputeShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const;

	void VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) const;
	void CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) const;

	void CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, 
	                               ID3D11UnorderedAccessView * const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) const;

	void CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) const;
	
	void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) const;


	HRESULT CreateQuery(const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery) const;
	void End(ID3D11Asynchronous *pAsync) const;
	HRESULT GetData(ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags) const;


	//d3d10
	HRESULT CreateMesh(CONST D3D10_INPUT_ELEMENT_DESC *pDeclaration, UINT DeclCount, LPCSTR pPositionSemantic, 
	                   UINT VertexCount, UINT FaceCount, UINT Options, ID3DX10Mesh **ppMesh) const;
	HRESULT OptimizeMesh(ID3DX10Mesh *mesh, UINT Flags, UINT *pFaceRemap, LPD3D10BLOB *ppVertexRemap, const UINT initialVertexCount) const;




private:
	HRESULT PrepareDevicesAndSwapChain(const DXGI_MODE_DESC &mode, const bool windowed, const bool aa, const HWND window, const UINT totalBackBuffers);
	HRESULT PrepareDepthStencilBuffer();
	HRESULT PrepareViewPort();

private:
	static const UINT SAMPLE_COUNT = 8;

	ID3D11Device *m_device11;
	ID3D11DeviceContext *m_deviceContext;
	IDXGISwapChain *m_swapChain;

	ID3D10Device *m_device10;

	ID3D11RenderTargetView *m_renderTargetView;
	ID3D11DepthStencilView *m_depthStencilView;

	D3D11_VIEWPORT m_vp;

	bool m_vsync;

	UINT m_MSsampleCount;
	UINT m_MSqualityLevel;

	IFW1FontWrapper *m_font;

	bool m_ready;
};


inline D3D11_VIEWPORT const &D3DDevicesManager::GetViewPort() const
{
	return m_vp;
}

//antes de liberar una fullscreen swapchain debemos setearla a modo windowed
inline void D3DDevicesManager::PrepareForExit()
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::PrepareForExit"); return; }

	m_swapChain->SetFullscreenState(false, NULL);
}

inline void D3DDevicesManager::DrawString(const WCHAR *text, FLOAT FontSize, FLOAT X, FLOAT Y, UINT32 Color, UINT Flags) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::DrawString"); return; }

	m_font->DrawString(m_deviceContext, text, FontSize, X, Y, Color, Flags);
}

inline HRESULT D3DDevicesManager::ApplyEffectPass(ID3DX11EffectPass *effectPass, const UINT flags) const
{
	_ASSERT(m_ready && effectPass);

	if(!m_ready || !effectPass) { 
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::ApplyEffectPass");
		return E_FAIL; 
	}

	HRESULT hr;

	if(FAILED(hr = effectPass->Apply(flags, m_deviceContext) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::ApplyEffectPass-->Apply");
	
	return hr;
}

inline void D3DDevicesManager::IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY Topology ) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::IASetPrimitiveTopology"); return; }

	m_deviceContext->IASetPrimitiveTopology( Topology );
}

//si no especificas ningún parámetro pone el estado por defecto
inline void D3DDevicesManager::OMSetBlendState(ID3D11BlendState *blendState, const FLOAT *BlendFactor, UINT SampleMask) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::OMSetBlendState"); return; }

	if(BlendFactor == NULL)
	{
		float blendFactors[] = {0.0f, 0.0f, 0.0f, 0.0f};
		m_deviceContext->OMSetBlendState(blendState, blendFactors, SampleMask);
	}
	else
		m_deviceContext->OMSetBlendState(blendState, BlendFactor, SampleMask);
}

inline void D3DDevicesManager::OMSetDepthStencilState(ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::OMSetDepthStencilState"); return; }

	m_deviceContext->OMSetDepthStencilState( pDepthStencilState, StencilRef );
}

inline void D3DDevicesManager::RSSetState(ID3D11RasterizerState *pRasterizerState) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::RSSetState"); return; }

	m_deviceContext->RSSetState( pRasterizerState );
}

inline void D3DDevicesManager::IASetInputLayout(ID3D11InputLayout *pInputLayout) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::IASetInputLayout"); return; }

	m_deviceContext->IASetInputLayout( pInputLayout );
}


inline HRESULT D3DDevicesManager::Present() const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::Present"); return E_FAIL; }

	HRESULT hr;

	const UINT interval = m_vsync ? 1 : 0;
	hr = m_swapChain->Present(interval, 0);

	if(FAILED(hr))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::Present --> IDXGISwapChain::Present");

	return hr;
}

inline void D3DDevicesManager::ClearBackBuffer(float r, float g, float b, float a) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::ClearBackBuffer"); return; }

	float ClearColor[4] = { r, g, b, a }; // red, green, blue, alpha
	m_deviceContext->ClearRenderTargetView( m_renderTargetView, ClearColor );
}

inline void D3DDevicesManager::ClearDepthStencilBuffer(const float depth) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::ClearDepthStencilBuffer"); return; }

	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, 0);
}

inline void D3DDevicesManager::ResetRenderingToBackBuffer() const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::ResetRenderingToBackBuffer"); return; }

	m_deviceContext->RSSetViewports(1, &m_vp);
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
}

inline HRESULT D3DDevicesManager::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D11DepthStencilState **ppDepthStencilState) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateDepthStencilState"); return E_FAIL; }

	HRESULT hr;

	if( FAILED( hr = m_device11->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateDepthStencilState --> ID3D11Device::CreateDepthStencilState");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc, ID3D11RasterizerState **ppRasterizerState) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateRasterizerState"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateRasterizerState(pRasterizerDesc, ppRasterizerState )))
		DXGI_D3D_ErrorWarning(hr, L" D3DDevicesManager::CreateRasterizerState --> ID3D11Device::CreateRasterizerState");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc, ID3D11BlendState **ppBlendState) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateBlendState"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateBlendState(pBlendStateDesc, ppBlendState )))
		DXGI_D3D_ErrorWarning(hr, L" D3DDevicesManager::CreateBlendState --> ID3D11Device::CreateBlendState");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements,
                                                    const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, 
                                                    ID3D11InputLayout **ppInputLayout) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateInputLayout"); return E_FAIL; }

	HRESULT hr;

	if(FAILED(hr = m_device11->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateInputLayout --> ID3D11Device::CreateInputLayout");
	
	return hr;
}

inline HRESULT D3DDevicesManager::CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateBuffer"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateBuffer(pDesc, pInitialData, ppBuffer) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DdevicesManager::CreateBuffer --> ID3D11Device::CreateBuffer");

	return hr;
}
inline HRESULT D3DDevicesManager::CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateShaderResourceView"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateShaderResourceView(pResource, pDesc, ppSRView) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DdevicesManager::CreateShaderResourceView --> ID3D11Device::CreateShaderResourceView");

	return hr;
}
inline HRESULT D3DDevicesManager::CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateUnorderedAccessView"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateUnorderedAccessView(pResource, pDesc, ppUAView) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DdevicesManager::CreateUnorderedAccessView --> ID3D11Device::CreateUnorderedAccessView");

	return hr;

}

inline HRESULT D3DDevicesManager::CreateEffectFromMemory(void *pData, SIZE_T DataLength, UINT FXFlags, ID3DX11Effect **ppEffect) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateEffectFromMemory"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = D3DX11CreateEffectFromMemory(pData, DataLength, FXFlags, m_device11, ppEffect) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::D3DX11CreateEffectFromMemory --> D3DX11CreateEffectFromMemory");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateVertexShader"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateVertexShader --> ID3D11Device::CreateVertexShader");

	return hr;
}

inline HRESULT D3DDevicesManager::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreatePixelShader"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreatePixelShader --> ID3D11Device::CreatePixelShader");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength,  ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppComputeShader) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateComputeShader"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateComputeShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateComputeShader --> ID3D11Device::CreateComputeShader");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateTexture2D"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateTexture2D(pDesc, pInitialData, ppTexture2D) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateTexture2D --> ID3D11Device::CreateTexture2D");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,  ID3D11RenderTargetView **ppRTView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateRenderTargetView"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateRenderTargetView(pResource, pDesc, ppRTView) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateRenderTargetView --> ID3D11Device::CreateRenderTargetView");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateDepthStencilView"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = m_device11->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateDepthStencilView --> ID3D11Device::CreateDepthStencilView");

	return hr;
}

inline void D3DDevicesManager::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews,ID3D11DepthStencilView *pDepthStencilView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::OMSetRenderTargets"); return; }

	m_deviceContext->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
}

inline void D3DDevicesManager::OMGetDepthStencilState(ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::OMGetDepthStencilState"); return; }

	m_deviceContext->OMGetDepthStencilState(ppDepthStencilState, pStencilRef);
}

inline void D3DDevicesManager::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT *pViewports) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::RSSetViewports"); return; }

	m_deviceContext->RSSetViewports(NumViewports, pViewports);
}
inline void D3DDevicesManager::RSGetScissorRects(UINT *pNumRects, D3D11_RECT *pRects) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::RSGetScissorRects"); return; }

	m_deviceContext->RSGetScissorRects(pNumRects, pRects);
}
inline void D3DDevicesManager::RSGetViewports(UINT *pNumViewports, D3D11_VIEWPORT *pViewports) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::RSGetViewports"); return; }

	m_deviceContext->RSGetViewports(pNumViewports, pViewports);
}

inline void D3DDevicesManager::RSGetState(ID3D11RasterizerState **ppRasterizerState) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::RSGetState"); return; }

	m_deviceContext->RSGetState(ppRasterizerState);
}

inline void D3DDevicesManager::RSSetScissorRects(UINT NumRects, const D3D11_RECT *pRects) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::RSSetScissorRects"); return; }

	m_deviceContext->RSSetScissorRects(NumRects, pRects);
}

inline void D3DDevicesManager::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::OMGetRenderTargets"); return; }

	m_deviceContext->OMGetRenderTargets(NumViews, ppRenderTargetViews, ppDepthStencilView);
}

inline void D3DDevicesManager::ClearDepthStencilView(ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::ClearDepthStencilView"); return; }

	m_deviceContext->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
}

inline void D3DDevicesManager::ClearRenderTargetView(ID3D11RenderTargetView *pRenderTargetView, const FLOAT *ColorRGBA) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::ClearRenderTargetView"); return; }

	m_deviceContext->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}

inline void D3DDevicesManager::GenerateMips(ID3D11ShaderResourceView *pShaderResourceView) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::GenerateMips"); return; }

	m_deviceContext->GenerateMips(pShaderResourceView);
}

inline void D3DDevicesManager::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::IASetVertexBuffers"); return; }

	m_deviceContext->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

inline void D3DDevicesManager::IASetIndexBuffer(ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::IASetIndexBuffer"); return; }

	m_deviceContext->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}



inline void D3DDevicesManager::Draw(UINT VertexCount, UINT StartVertexLocation) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::Draw"); return; }

	m_deviceContext->Draw(VertexCount, StartVertexLocation);
}

inline void D3DDevicesManager::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::DrawIndexed"); return; }

	m_deviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

inline HRESULT D3DDevicesManager::CreateMesh(CONST D3D10_INPUT_ELEMENT_DESC *pDeclaration, UINT DeclCount, LPCSTR pPositionSemantic, 
                                             UINT VertexCount, UINT FaceCount, UINT Options, ID3DX10Mesh **ppMesh) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateMesh"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = D3DX10CreateMesh(m_device10, pDeclaration, DeclCount, pPositionSemantic, VertexCount, FaceCount, Options, ppMesh) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateMesh --> D3DX10CreateMesh");

	return hr;
}

inline HRESULT D3DDevicesManager::OptimizeMesh(ID3DX10Mesh *mesh, UINT Flags, UINT *pFaceRemap, LPD3D10BLOB *ppVertexRemap, const UINT initialVertexCount) const
{
	_ASSERT(m_ready && mesh);

	if(!m_ready || !mesh) { 
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::OptimizeMesh"); 
		return E_FAIL; 
	}

	HRESULT hr;

	if(FAILED( hr = mesh->Optimize(Flags, pFaceRemap, ppVertexRemap) ))
	{
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::OptimizeMesh --> ID3DX10Mesh::Optimize");
		return hr;
	}

	//memory leak fix: ID3DX10Mesh::Optimize deja memory leaks en el ID3D10Device. 
	//Pero sólo cuando se duplican vértices (con el flag D3DX10_MESHOPT_DO_NOT_SPLIT no quedan leaks)
	if(initialVertexCount != mesh->GetVertexCount()) {
		m_device10->Release();
		m_device10->Release();	//Optimize aumenta la ref count en dos para el caso de vértices duplicados. Aquí lo arreglamos.
	}

	return hr;
}

inline HRESULT D3DDevicesManager::CreateShaderResourceViewFromFileD3D11(LPCTSTR pSrcFile, D3DX11_IMAGE_LOAD_INFO *pLoadInfo, ID3DX11ThreadPump *pPump,
                                                                        ID3D11ShaderResourceView **ppShaderResourceView, HRESULT *pHResult) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateShaderResourceViewFromFileD3D11"); return E_FAIL; }

	HRESULT hr;

	//tercer parametro es NULL para leer las caracteristicas de la textura cuando esté cargada. 
	if(FAILED( hr = D3DX11CreateShaderResourceViewFromFile(m_device11, pSrcFile, pLoadInfo, pPump, ppShaderResourceView, pHResult) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateShaderResourceViewFromFileD3D11 --> D3DX11CreateShaderResourceViewFromFile");

	return hr;
}

inline HRESULT D3DDevicesManager::CreateTextureFromFileD3D11(LPCTSTR pSrcFile, D3DX11_IMAGE_LOAD_INFO *pLoadInfo,  ID3DX11ThreadPump *pPump,
                                                             ID3D11Resource **ppTexture,  HRESULT *pHResult) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateTextureFromFileD3D11"); return E_FAIL; }

	HRESULT hr;

	if(FAILED( hr = D3DX11CreateTextureFromFile(m_device11, pSrcFile, pLoadInfo, pPump, ppTexture, pHResult) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::CreateTextureFromFileD3D11 --> D3DX11CreateTextureFromFile");

	return hr;
}

inline void D3DDevicesManager::CopyResource(ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CopyResource"); return; }

	m_deviceContext->CopyResource(pDstResource, pSrcResource);
}

inline HRESULT D3DDevicesManager::Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::Map"); return E_FAIL; }

	HRESULT hr;

	if(FAILED(hr = m_deviceContext->Map(pResource, Subresource, MapType, MapFlags, pMappedResource) ))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::Map --> ID3D11DeviceContext::Map");
	
	return hr;
}

inline void D3DDevicesManager::Unmap(ID3D11Resource *pResource, UINT Subresource) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::Unmap"); return; }

	m_deviceContext->Unmap(pResource, Subresource);
}


inline void D3DDevicesManager::VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::VSSetShader"); return; }

	m_deviceContext->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);
}
inline void D3DDevicesManager::PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::PSSetShader"); return; }

	m_deviceContext->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);
}
inline void D3DDevicesManager::GSSetShader(ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::GSSetShader"); return; }

	m_deviceContext->GSSetShader(pShader, ppClassInstances, NumClassInstances);
}
inline void D3DDevicesManager::DSSetShader(ID3D11DomainShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::DSSetShader"); return; }

	m_deviceContext->DSSetShader(pShader, ppClassInstances, NumClassInstances);
}
inline void D3DDevicesManager::HSSetShader(ID3D11HullShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::HSSetShader"); return; }

	m_deviceContext->HSSetShader(pShader, ppClassInstances, NumClassInstances);
}
inline void D3DDevicesManager::CSSetShader(ID3D11ComputeShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CSSetShader"); return; }

	m_deviceContext->CSSetShader(pShader, ppClassInstances, NumClassInstances);
}


inline void D3DDevicesManager::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::VSSetShaderResources"); return; }

	m_deviceContext->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

inline void D3DDevicesManager::CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CSSetShaderResources"); return; }

	m_deviceContext->CSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

inline void D3DDevicesManager::CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, 
                                                         ID3D11UnorderedAccessView * const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CSSetUnorderedAccessViews"); return; }

	m_deviceContext->CSSetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

inline void D3DDevicesManager::CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CSSetConstantBuffers"); return; }

	m_deviceContext->CSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

inline void D3DDevicesManager::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::Dispatch"); return; }

	m_deviceContext->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

inline HRESULT D3DDevicesManager::CreateQuery(const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::CreateQuery"); return E_FAIL; }

	HRESULT hr;

	if(FAILED(hr = m_device11->CreateQuery( pQueryDesc, ppQuery ))) 
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::Map --> CreateQuery");

	return hr;
}

inline void D3DDevicesManager::End(ID3D11Asynchronous *pAsync) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::End"); return; }

	m_deviceContext->End(pAsync);
}

inline HRESULT D3DDevicesManager::GetData(ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags) const
{
	_ASSERT(m_ready);
	if(!m_ready) { MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::GetData"); return E_FAIL; }

	return m_deviceContext->GetData(pAsync, pData, DataSize, GetDataFlags);
}

}

#endif