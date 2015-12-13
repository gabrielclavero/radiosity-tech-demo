//------------------------------------------------------------------------------------------
// File: OmniShadowMap.h
//
// Esta clase carga un shader que renderiza el depth map para el algoritmo de sombreado de
// escenas con luces omnidireccionales. Prepara un viewport, variables asociadas y configura
// lo necesario para dicha renderización.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef OMNI_SHADOW_MAP_H
#define OMNI_SHADOW_MAP_H

#include "ShadowMap.h"

using std::wstring;

namespace DTFramework
{

class OmniShadowMap : public ShadowMap
{
public:
	OmniShadowMap(const D3DDevicesManager &d3d, const UINT width, const UINT height);
	virtual ~OmniShadowMap();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	virtual HRESULT ComputeShadowMap(const Scene &scene, const Light &light);

	virtual ID3D11ShaderResourceView *GetShadowMap() const;

	//debug
	#ifdef DEBUG_TEXTURES
		virtual HRESULT DebugDrawShadow();
	#endif

private:
	D3DXMATRIX m_cubeMapViewAdjust[6];       // ajuste para las view matrices cuando rendericemos el cube map
	D3DXMATRIX m_proj;                      // projection matrix para cubic map rendering

	ID3DX11EffectMatrixVariable *m_viewMatrixVariable;
	ID3DX11EffectMatrixVariable *m_worldMatrixVariable;
	ID3DX11EffectMatrixVariable *m_projMatrixVariable;

	ID3DX11EffectVectorVariable *m_lightPos;
	ID3DX11EffectScalarVariable *m_lightRange;
};

inline ID3D11ShaderResourceView *OmniShadowMap::GetShadowMap() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"OmniShadowMap::GetShadowMap");
		return NULL;
	}

	if(m_renderableTexture)
		return m_renderableTexture->GetColorTexture();
	else
		return NULL;
}

}

#endif