//------------------------------------------------------------------------------------------
// File: Material.h
//
// La clase Material contiene la descripción de las propiedades lumínicas de un material, ruta
// de las texturas difusa, normal, sus shader resource views asociados,
// y nombre de las techniques con las que debe renderizarse.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef MATERIAL_H
#define MATERIAL_H

#include "Utility.h"

using std::wstring;
using std::string;

namespace DTFramework
{

struct MaterialLightProperties
{
	D3DXCOLOR emissive;

	D3DXVECTOR3 ambient;
	D3DXVECTOR3 specular;
	D3DXVECTOR3 diffuse;

	float power;

	float shininess;
	float alpha;

	bool bSpecular;
};

class Material
{
public:
	Material();
	~Material();

	HRESULT SetName(const WCHAR * const name=NULL);

	const wstring &GetName() const;
	const wstring &GetDiffuseTextureName() const;
	const wstring &GetNormalTextureName() const;

	float GetAlpha() const;
	const MaterialLightProperties &GetLightProperties() const;

	ID3D11ShaderResourceView *GetDiffuseTextureSRV() const;
	ID3D11ShaderResourceView *GetNormalTextureSRV() const;

	const string &GetTechniqueName() const;

	//lighting properties
	void SetSpecular(const D3DXVECTOR3 &v);
	void SetAmbient(const D3DXVECTOR3 &v);
	void SetDiffuse(const D3DXVECTOR3 &v);
	void SetbSpecular(const bool b);
	void SetAlpha(const float f);
	void SetShininess(const float s);

	//nombres de texturas
	void SetDiffuseTextureName(const wstring &t);
	void SetNormalTextureName(const wstring &t);

	//shader resource views de texturas
	void SetDiffuseTextureSRV(ID3D11ShaderResourceView * const srv);
	void SetNormalTextureSRV(ID3D11ShaderResourceView * const srv);
	
	void SetTechniqueName(const string &t);


private:
	MaterialLightProperties m_lightProperties;

	wstring m_nameMaterial;

	wstring m_strTexture;
	wstring m_strNormalTexture;

	string m_technique;


	ID3D11ShaderResourceView *m_textureRV11;
	ID3D11ShaderResourceView *m_normalTextureRV11;
};

inline Material::Material()
: m_textureRV11(0), m_normalTextureRV11(0)
{
	ZeroMemory(&m_lightProperties, sizeof(MaterialLightProperties));
}

inline HRESULT Material::SetName(const WCHAR * const name_material)
{
	if(name_material != NULL) {
		m_nameMaterial = wstring(name_material);
	} 
	else {	//default material
		m_nameMaterial = wstring(L"default");
		
		m_lightProperties.ambient = D3DXVECTOR3(0.2f, 0.2f, 0.2f);
		m_lightProperties.diffuse = D3DXVECTOR3(0.8f, 0.8f, 0.8f);
		m_lightProperties.specular = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
		m_lightProperties.shininess = 0;
		m_lightProperties.alpha = 1.0f;
		m_lightProperties.bSpecular = true;
	}

	return S_OK;
}

inline Material::~Material()
{
	SAFE_RELEASE(m_textureRV11);
	SAFE_RELEASE(m_normalTextureRV11);
}

inline const wstring &Material::GetName() const
{
	return m_nameMaterial;
}
inline const wstring &Material::GetDiffuseTextureName() const
{
	return m_strTexture;
}
inline const wstring &Material::GetNormalTextureName() const
{
	return m_strNormalTexture;
}
inline const MaterialLightProperties &Material::GetLightProperties() const
{
	return m_lightProperties;
}
inline float Material::GetAlpha() const 
{ 
	return m_lightProperties.alpha;
}
inline void Material::SetSpecular(const D3DXVECTOR3 &v)
{
	m_lightProperties.specular = v;
}
inline void Material::SetAmbient(const D3DXVECTOR3 &v)
{
	m_lightProperties.ambient = v;
}
inline void Material::SetDiffuse(const D3DXVECTOR3 &v)
{
	m_lightProperties.diffuse = v;
}
inline void Material::SetbSpecular(const bool b)
{
	m_lightProperties.bSpecular = b;
}
inline void Material::SetAlpha(const float f)
{
	m_lightProperties.alpha = f;
}
inline void Material::SetDiffuseTextureName(const wstring &t)
{
	m_strTexture = t;
}
inline void Material::SetNormalTextureName(const wstring &t)
{
	m_strNormalTexture = t;
}
inline void Material::SetDiffuseTextureSRV(ID3D11ShaderResourceView * const srv)
{
	m_textureRV11 = srv;
}
inline void Material::SetNormalTextureSRV(ID3D11ShaderResourceView * const srv)
{
	m_normalTextureRV11 = srv;
}
inline void Material::SetShininess(const float s)
{
	m_lightProperties.shininess = s;
}
inline void Material::SetTechniqueName(const string &t)
{
	m_technique = t;
}
inline ID3D11ShaderResourceView *Material::GetDiffuseTextureSRV() const
{
	return m_textureRV11;
}
inline ID3D11ShaderResourceView *Material::GetNormalTextureSRV() const
{
	return m_normalTextureRV11;
}
inline const string &Material::GetTechniqueName() const
{
	return m_technique;
}

}

#endif;
