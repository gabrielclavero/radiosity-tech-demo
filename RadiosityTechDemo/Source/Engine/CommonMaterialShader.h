//------------------------------------------------------------------------------------------
// File: CommonMaterialShader.h
//
// Actúa de interfaz hacia los shaders que renderizan caras con cualquier material aplicado.
// Permite asignar valores a las variables del mismo y designar la technique que se 
// utilizará. Ver Shaders/commonMaterialShader.fx para inspeccionar el código de los shaders.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef COMMON_MATERIAL_SHADER_H
#define COMMON_MATERIAL_SHADER_H

#include "Utility.h"
#include "CompiledShader.h"
#include "Material.h"
#include "Light.h"
#include "D3DDevicesManager.h"

using std::vector;
using std::wstring;

namespace DTFramework
{

class CommonMaterialShader
{
public:
	CommonMaterialShader(const D3DDevicesManager &d3d);
	~CommonMaterialShader();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init();

	ID3DX11EffectTechnique *GetTechnique() const;

	HRESULT SetShaderVariablesPerFrame(const D3DXVECTOR3 &camPos, const LightProperties * const light, ID3D11ShaderResourceView *shadowMap, const UINT activeLights);

	HRESULT SetShaderVariablesPerObject(const D3DMATRIX &wvp, const D3DMATRIX * const lightWVP = NULL, ID3D11ShaderResourceView * const GIMeshData=NULL);

	HRESULT SetShaderVariablesPerMaterial(const Material &Material);

	HRESULT SetTechnique(const Material &material, const bool useGI=true);

private:
	const D3DDevicesManager &m_d3dManager;

	CompiledShader m_shader;
	
	ID3DX11EffectTechnique *m_technique;

	//texturas
	ID3DX11EffectShaderResourceVariable *m_diffuseTexVariable;
	ID3DX11EffectShaderResourceVariable *m_normalTexVariable;

	//gi
	ID3DX11EffectShaderResourceVariable *m_GIBuffer;

	//material light properties
	ID3DX11EffectVectorVariable *m_ambient;
	ID3DX11EffectVectorVariable *m_diffuse;
	ID3DX11EffectVectorVariable *m_specular;
	ID3DX11EffectScalarVariable *m_opacity;
	ID3DX11EffectScalarVariable *m_specularPower;

	//matrices
	ID3DX11EffectMatrixVariable *m_WVPMatrixVariable;

	//camera
	ID3DX11EffectVectorVariable *m_cameraPosition;

	//lights
	ID3DX11EffectVariable		*m_shaderLight;
	ID3DX11EffectScalarVariable	*m_activeLightsVariable;

	//shadow 
	ID3DX11EffectShaderResourceVariable	*m_shadowDepthMapVariable;		
	ID3DX11EffectMatrixVariable			*m_lightWVPVariable;			//view projection matrix de la luz para el depth test con el shadow depth map
	ID3DX11EffectShaderResourceVariable	*m_omniShadowDepthMapVariable;	

	bool m_ready;
};

inline ID3DX11EffectTechnique *CommonMaterialShader::GetTechnique() const
{
	return m_technique;
}

}

#endif
