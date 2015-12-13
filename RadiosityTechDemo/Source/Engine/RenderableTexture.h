//------------------------------------------------------------------------------------------
// File: RenderableTexture.h
//
// Crea dos texturas que pueden ser el render target y depth buffer respectivamente de 
// alguna renderización.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef RENDERABLE_TEXTURE_H
#define RENDERABLE_TEXTURE_H

#include "Utility.h"
#include "D3DDevicesManager.h"
#include "D3D11Resources.h"

#ifdef DEBUG_TEXTURES
	#include "CompiledShader.h"
#endif

namespace DTFramework
{

class RenderableTexture
{
public:
	RenderableTexture(const D3DDevicesManager &d3d);
	~RenderableTexture();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init(const UINT width, const UINT height, const bool depthOnly=false, const float minDepth=0.0f, const DXGI_FORMAT format=RENDERABLE_TEXTURE_PIXEL_FORMAT, 
	             const bool renderTargetOnly=false, const UINT arraySize=1, const bool isCube=false);

	ID3D11ShaderResourceView *GetColorTexture() const;
	ID3D11ShaderResourceView *GetDepthTexture() const;
	
	ID3D11RenderTargetView *GetRenderTargetView() const;
	ID3D11DepthStencilView *GetDepthStencilTarget() const;

	void Begin(const float * const color = NULL);
	void End();

	void SetViewPortMinDepth(const float depth);

	//debug
	#ifdef DEBUG_TEXTURES
		HRESULT DebugDrawTexture(const bool renderDepthTexture=false, const bool renderCube=false);
	#endif

private:
	#ifdef DEBUG_TEXTURES
		HRESULT BuildNDCQuad();
	#endif

	HRESULT InitColorTexture(const DXGI_FORMAT format, const UINT arraySize=1, const bool isCube=false);
	HRESULT InitDepthTexture(const UINT arraySize=1, const bool isCube=false);

private:
	static const DXGI_FORMAT RENDERABLE_TEXTURE_PIXEL_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

	const D3DDevicesManager &m_d3dManager;

	Texture2D_NOAA *m_colorTexture;
	Texture2D_NOAA *m_depthTexture;

	UINT m_width;
	UINT m_height;
	
	D3D11_VIEWPORT *m_viewPort;

	bool m_ready;

	//debug
	#ifdef DEBUG_TEXTURES
		CompiledShader shader;
		VertexBuffer *m_NDCQuadVB;
		ID3DX11EffectTechnique *drawTextureTechnique;
		ID3DX11EffectShaderResourceVariable *drawTextureVariable;
		ID3DX11EffectShaderResourceVariable *drawTextureCubeVariable;
		ID3DX11EffectScalarVariable	*singleDepthVariable;
	#endif
};


inline ID3D11RenderTargetView  *RenderableTexture::GetRenderTargetView() const
{
	return (m_colorTexture ? m_colorTexture->GetRenderTargetView() : NULL);
}

inline ID3D11DepthStencilView *RenderableTexture::GetDepthStencilTarget() const
{
	return (m_depthTexture ? m_depthTexture->GetDepthStencilView() : NULL);
}

inline ID3D11ShaderResourceView *RenderableTexture::GetColorTexture() const
{
	return (m_colorTexture ? m_colorTexture->GetShaderResourceView() : NULL);
}
inline ID3D11ShaderResourceView *RenderableTexture::GetDepthTexture() const
{
	return (m_depthTexture ? m_depthTexture->GetShaderResourceView() : NULL);
}

inline void RenderableTexture::SetViewPortMinDepth(const float depth)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"RenderableTexture::SetViewPortMinDepth");
		return;
	}

	if(m_viewPort)
		m_viewPort->MinDepth = depth;
}


}

#endif