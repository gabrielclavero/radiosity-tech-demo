//------------------------------------------------------------------------------------------
// File: ShadowMap.h
//
// Clase abstracta que comprende la funcionalidad general que se necesita para algoritmos
// que implementen la técnica de shadow mapping.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "Utility.h"
#include "CompiledShader.h"
#include "RenderableTexture.h"
#include "Light.h"
#include "D3DDevicesManager.h"
#include "Scene.h"
#include "D3D11DeviceStates.h"
#include "InputLayouts.h"

using std::wstring;

namespace DTFramework
{

class ShadowMap
{
public:
	ShadowMap(const D3DDevicesManager &d3d, const UINT width, const UINT height);
	virtual ~ShadowMap();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init() = 0;

	virtual HRESULT ComputeShadowMap(const Scene &scene, const Light &light) = 0;

	virtual ID3D11ShaderResourceView *GetShadowMap() const = 0;

	//debug
	#ifdef DEBUG_TEXTURES
		virtual HRESULT DebugDrawShadow() = 0;
	#endif

protected:
	HRESULT PrepareShaderAndDeviceStates(const wstring &shaderFile);
	HRESULT PrepareForRender();
	HRESULT Render(const Scene &scene);

protected:
	const D3DDevicesManager &m_d3dManager;

	const UINT m_width;
	const UINT m_height;

	CompiledShader m_shader;

	RenderableTexture *m_renderableTexture;

	//input layouts
	InputLayouts m_inputLayouts;

	//device states
	D3D11DeviceStates *m_deviceStates;

	ID3DX11EffectTechnique *m_technique;

	//variables temporales
	D3D11_VIEWPORT m_oldViewports[1];
	ID3D11RenderTargetView *m_oldRenderTargets[1];
	ID3D11DepthStencilView *m_oldDepthStencilViews[1];
	ID3D11RasterizerState *m_oldRasterizerState;
	ID3D11DepthStencilState *m_oldDepthStencilState;
	UINT m_oldStencilRef;

	bool m_ready;
};

}

#endif