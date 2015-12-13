//------------------------------------------------------------------------------------------
// File: Skybox.h
//
// Esta clase permite renderizar un sky box con el modelo estándar del CIE. 
// Ver Shaders/skyTexture.fx y Shaders/skyBox.fx.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef SKY_BOX_H
#define SKY_BOX_H

#include "Utility.h"
#include "CompiledShader.h"
#include "InputLayouts.h"
#include "RenderableTexture.h"
#include "D3D11DeviceStates.h"
#include "D3DDevicesManager.h"
#include "D3D11Resources.h"

namespace DTFramework
{

class Skybox
{
public:
	Skybox(const D3DDevicesManager &d3d);
	~Skybox();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init();

	HRESULT Render(const D3DXMATRIX &view, const D3DXMATRIX &projection, const D3DXVECTOR3 &sunDir, const D3DXVECTOR3 bias = D3DXVECTOR3(1,1,1), const bool useLowResTexture=false);

	//crear textura de diferente resolución que la del backbuffer, para renderizaciones más pequeñas
	HRESULT CreateLowResTexture(const UINT width, const UINT height);
	
private:
	static const UINT NUM_INDICES;
	static const UINT NUM_INDICES_SKYBOX;

	const D3DDevicesManager &m_d3dManager;

	CompiledShader m_skyTextureEffect;
	CompiledShader m_skyBoxEffect;

	InputLayouts m_inputLayouts;

	RenderableTexture *m_skyTexture;
	RenderableTexture *m_skyTextureLowRes;
	
	//skyTexture shader variables
	ID3DX11EffectVectorVariable *m_biasVariable;
	ID3DX11EffectVectorVariable *m_sunDirectionVariable;
	ID3DX11EffectMatrixVariable *m_viewMatrixVariable;
	ID3DX11EffectMatrixVariable *m_projMatrixVariable;

	//skyTexture technique
	ID3DX11EffectTechnique *m_technique;

	//skyTexture buffers
	VertexBuffer *m_vertexBuffer;
	IndexBuffer *m_indexBuffer;

	//skyBox shader variables
	ID3DX11EffectShaderResourceVariable *m_skyTextureVariable;

	//skyBox techniques
	ID3DX11EffectTechnique *m_skyBoxTechnique;

	//skyBox buffers
	VertexBuffer *m_skyBoxVertexBuffer;
	IndexBuffer *m_skyBoxIndexBuffer;

	//d3d states
	D3D11DeviceStates *m_deviceStates;

	bool m_ready;
};

}

#endif