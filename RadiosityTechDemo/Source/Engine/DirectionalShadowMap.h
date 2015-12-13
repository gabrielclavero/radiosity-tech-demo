//------------------------------------------------------------------------------------------
// File: DirectionalShadowMap.h
//
// Esta clase carga un shader que renderiza el depth map para el algoritmo de sombreado de
// escenas con luces direccionales. Prepara un viewport, variables asociadas y configura
// lo necesario para dicha renderización.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef DIRECTIONAL_SHADOW_MAP_H
#define DIRECTIONAL_SHADOW_MAP_H

#include "ShadowMap.h"

using std::wstring;

namespace DTFramework
{

class DirectionalShadowMap : public ShadowMap
{
public:
	DirectionalShadowMap(const D3DDevicesManager &d3d, const UINT width, const UINT height);
	virtual ~DirectionalShadowMap();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	//dada una escena y una luz direccional, computa el mapa de sombras
	virtual HRESULT ComputeShadowMap(const Scene &scene, const Light &light);

	virtual ID3D11ShaderResourceView *GetShadowMap() const;

	//debug
	#ifdef DEBUG_TEXTURES
		virtual HRESULT DebugDrawShadow();
	#endif

private:
	//shader variables
	ID3DX11EffectMatrixVariable *m_lightWVP;
};

inline ID3D11ShaderResourceView *DirectionalShadowMap::GetShadowMap() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"DirectionalShadowMap::GetShadowMap");
		return NULL;
	}

	if(m_renderableTexture)
		return m_renderableTexture->GetDepthTexture();
	else
		return NULL;
}

}

#endif