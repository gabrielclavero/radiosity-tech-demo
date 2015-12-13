//------------------------------------------------------------------------------------------
// File: CommonMaterialShader.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "CommonMaterialShader.h"

namespace DTFramework
{

CommonMaterialShader::CommonMaterialShader(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_shader(d3d), m_technique(0),
 m_diffuseTexVariable(0), m_normalTexVariable(0), 
  m_GIBuffer(0),
 m_ambient(0), m_diffuse(0), m_specular(0), m_opacity(0), m_specularPower(0),
 m_WVPMatrixVariable(0), m_cameraPosition(0), m_shaderLight(0), m_activeLightsVariable(0), 
 m_shadowDepthMapVariable(0), m_lightWVPVariable(0), m_omniShadowDepthMapVariable(0), m_ready(false)
{
	
}

CommonMaterialShader::~CommonMaterialShader()
{

}

HRESULT CommonMaterialShader::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CommonMaterialShader::Init");
		return E_FAIL;
	}

	HRESULT hr;

	if( !FAILED(hr = m_shader.LoadPrecompiledShader(BASE_SHADER_FILE)) ) 
	{
		ID3DX11Effect *tmp = m_shader.GetEffect();

		//matrices
		m_WVPMatrixVariable = tmp->GetVariableByName( "gWVP" )->AsMatrix();

		//texturas
		m_diffuseTexVariable = tmp->GetVariableByName( "gDiffuseTexture" )->AsShaderResource();
		m_normalTexVariable = tmp->GetVariableByName( "gNormalTexture" )->AsShaderResource();

		//GI
		m_GIBuffer = tmp->GetVariableByName( "gGILightInfoPerVertex" )->AsShaderResource();

		//material light properties
		m_ambient = tmp->GetVariableByName( "gMaterialAmbient" )->AsVector();
		m_diffuse = tmp->GetVariableByName( "gMaterialDiffuse" )->AsVector();
		m_specular = tmp->GetVariableByName( "gMaterialSpecular" )->AsVector();
		m_opacity = tmp->GetVariableByName( "gMaterialAlpha" )->AsScalar();
		m_specularPower = tmp->GetVariableByName( "gMaterialShininess" )->AsScalar();

		//otros
		m_cameraPosition = tmp->GetVariableByName( "gCameraPosition" )->AsVector();
		m_activeLightsVariable = tmp->GetVariableByName( "gActiveLights" )->AsScalar();

		//shadow
		m_lightWVPVariable = tmp->GetVariableByName( "gLightWVP" )->AsMatrix();			//matrices
		m_shadowDepthMapVariable = tmp->GetVariableByName( "gShadowMap" )->AsShaderResource();	//shadow maps
		m_omniShadowDepthMapVariable = tmp->GetVariableByName( "gOmniShadowMap" )->AsShaderResource();

		//luces
		m_shaderLight = tmp->GetVariableByName( "gLight" );
	}

	m_ready = true;

	return hr;
}

HRESULT CommonMaterialShader::SetTechnique(const Material &material, const bool useGI)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CommonMaterialShader::SetTechnique");
		return E_FAIL;
	}

	//setear la technique que usaremos en la siguiente draw call. 
	//useGI es true => usamos iluminación indirecta
	if(!useGI)
		m_technique = m_shader.GetEffect()->GetTechniqueByName(material.GetTechniqueName().c_str());
	else
		m_technique = m_shader.GetEffect()->GetTechniqueByName( (material.GetTechniqueName() + "GI").c_str() );

	if(!m_technique->IsValid()) {
		DXGI_D3D_ErrorWarning(E_FAIL, L"CommonMaterialShader::SetTechnique --> GetTechniqueByName");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CommonMaterialShader::SetShaderVariablesPerObject(const D3DMATRIX &wvp, const D3DMATRIX * const lightWVP, ID3D11ShaderResourceView * const GIMeshData)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CommonMaterialShader::SetShaderVariablesPerObject");
		return E_FAIL;
	}

	HRESULT hr;

	//Por simplicidad y porque las escenas siempre tienen un sólo objeto, World siempre es la identidad
	//world view projection matrix
	if(FAILED( hr = m_WVPMatrixVariable->SetMatrix((float *) &wvp))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"MaterialShader::SetShaderVariablesPerObject --> SetMatrix");
		return hr;
	}

	//world view projection matrix de la luz en el shader.
	if(lightWVP) 
	{
		if(FAILED( hr = m_lightWVPVariable->SetMatrix((float *) lightWVP ) ) ) 
		{
			DXGI_D3D_ErrorWarning(hr, L"MaterialShader::SetShaderVariablesPerObject --> SetMatrix");
			return hr;
		}
	}

	//GI light
	if(GIMeshData)
	{
		if(FAILED(hr = m_GIBuffer->SetResource( GIMeshData )))
			DXGI_D3D_ErrorWarning(hr, L"MaterialShader::SetShaderVariablesPerObject --> SetResource");
	}

	return hr;
}

HRESULT CommonMaterialShader::SetShaderVariablesPerMaterial(const Material &material)
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CommonMaterialShader::SetShaderVariablesPerMaterial");
		return E_FAIL;
	}

	HRESULT hr;

	//diffuse texture
	if(material.GetDiffuseTextureSRV()) 
	{
		//no hacer release del resource asociado a esta variable en el destructor porque se hace en Material::~Material y en Effect::~Effect
		//el resource existe en otro lado. Este es solo un puntero.
		if(FAILED(hr = m_diffuseTexVariable->SetResource( material.GetDiffuseTextureSRV() ))) 
		{
			DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetResource");
			return hr;
		}
	}

	//normal texture
	if(material.GetNormalTextureSRV()) 
	{
		//no hacer release del resource asociado a esta variable en el destructor porque se hace en Material::~Material y en Effect::~Effect
		if(FAILED(hr = m_normalTexVariable->SetResource( material.GetNormalTextureSRV() ))) 
		{
			DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetResource");
			return hr;
		}
	} 

	if(FAILED(hr = m_ambient->SetFloatVector( (float *) &(material.GetLightProperties().ambient) )))  
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetFloatVector");
		return hr;
	}
	if(FAILED(hr = m_diffuse->SetFloatVector( (float *) &(material.GetLightProperties().diffuse) ))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetFloatVector");
		return hr;
	}
	if(FAILED(hr = m_specular->SetFloatVector( (float *) &(material.GetLightProperties().specular) ))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetFloatVector");
		return hr; 
	}
	if(FAILED(hr = m_opacity->SetFloat( material.GetAlpha() ))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetFloat");
		return hr;
	}
	if(FAILED(hr = m_specularPower->SetFloat( material.GetLightProperties().shininess ))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerMaterial --> SetFloat");
		return hr;
	}

	return hr;
}

HRESULT CommonMaterialShader::SetShaderVariablesPerFrame(const D3DXVECTOR3 &camPos, const LightProperties * const light, ID3D11ShaderResourceView *shadowMap, const UINT activeLights)
{
	_ASSERT(m_ready);

	//si hay un shadow map también tiene que haber una luz. Y si hay una luz especificada quiere decir que debe haber al menos una luz activa, la vuelta de esto último no vale
	_ASSERT(!(shadowMap && !light) && !(light && activeLights <= 0));

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"CommonMaterialShader::SetShaderVariablesPerFrame");
		return E_FAIL;
	}

	if((shadowMap && !light) || (light && activeLights <= 0)) {
		MiscErrorWarning(INVALID_PARAMETER, L"CommonMaterialShader::SetShaderVariablesPerFrame");
		return E_INVALIDARG;
	}

	HRESULT hr;

	//camara
	if(FAILED( hr = m_cameraPosition->SetFloatVector( (float *) &camPos ) ) ) 
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerFrame --> SetFloatVector");
		return hr;
	}

	//luces. Si light está definido quiere decir que es el primer frame de esta ejecución o que la luz es dinámica. 
	//Si no está definido no quiere decir que no haya una luz activa, simplemente que no actualizaremos ahora. Las luces activas la indica activeLights.
	if(light)
	{
		if( FAILED(hr = m_shaderLight->SetRawValue( light, 0, sizeof(LightProperties) ) ) ) 
		{
			DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerFrame --> SetRawValue");
			return hr;
		}
	}

	//actualizar shadows maps en el shader para el depth test de la sombra, solo si hay un shadow map definido (y por lo tanto una luz y por lo tanto una luz activa)
	if(shadowMap) 
	{
		if((*light).type == POINT_LIGHT)
		{
			//no hacer release del resource asociado a esta variable en el destructor porque se hace en ShadowMap::~ShadowMap y en Effect::~Effect
			if(FAILED(hr = m_omniShadowDepthMapVariable->SetResource( shadowMap ))) 
			{
				DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerFrame --> SetResource");
				return hr;
			}
		} 
		else
		{
			//no hacer release del resource asociado a esta variable en el destructor porque se hace en ShadowMap::~ShadowMap y en Effect::~Effect
			if(FAILED(hr = m_shadowDepthMapVariable->SetResource( shadowMap ))) 
			{
				DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerFrame --> SetResource");
				return hr;
			}
		}
	}
	
	if(FAILED(hr = m_activeLightsVariable->SetInt(activeLights))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"CommonMaterialShader::SetShaderVariablesPerFrame --> SetInt");
		return hr;
	}

	return hr;
}

}
