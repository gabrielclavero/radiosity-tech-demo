//------------------------------------------------------------------------------------------
// File: Light.h
//
// Light especifica las propiedades de una luz en la escena.
// Posición, dirección, tipo de luz, color ambiental, difuso, especular, etcétera.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef LIGHT_H
#define LIGHT_H

#include <D3DX10math.h>

namespace DTFramework
{

enum LightType { DIRECTIONAL_LIGHT, POINT_LIGHT };

//el padding esta allí para que la estructura sea similar a la estructura Light en el shader. 
struct LightProperties
{
	D3DXVECTOR3 pos;
	float pad1;
	D3DXVECTOR3 dir;
	float pad2;
	D3DXCOLOR ambient;
	D3DXCOLOR diffuse;
	D3DXCOLOR specular;
	D3DXVECTOR3 att;	//attenuation
	float range;
	int type;
	float shadowMapBias;
	int on;				//1 si la luz está encendida, 0 si no
	float pad3;
};

class Light
{
public:
	Light()
	: m_type(DIRECTIONAL_LIGHT), m_zNear(0), m_zFar(0), m_dirLightWidth(0), m_dirLightHeight(0), m_isDynamic(0), m_update(true)
	{
		ZeroMemory(&m_lightProperties, sizeof(LightProperties));
		m_lightProperties.on = 1;
	}

	void SetDirection(const D3DXVECTOR3 &dir);
	
	void SetPosition(const D3DXVECTOR3 &pos);
	
	void SetProperties(const LightProperties &properties, const LightType type);		//variables usadas en el shader
	
	void SetDynamic(const bool b);
	
	void SetZNear(const float f);
	
	void SetZFar(const float f);
	
	void SetDirectionalLightVolume(const float w, const float h);
	
	void SetOnOffState();
	
	void SetOnState();
	
	void SetOffState();
	
	void OnOffUpdateDone();
	

	const D3DXVECTOR3 &GetPosition() const;
	
	const D3DXVECTOR3 &GetDirection() const;
	
	LightType GetType() const;
	
	float GetRange() const;
	
	const LightProperties &GetProperties() const;
	
	float GetShadowMapBias() const;
	
	bool IsOn() const;
	

	bool IsDynamic() const;
	
	bool ShouldUpdate() const;
	
	float GetZNear() const;
	
	float GetZFar() const;
	
	float GetLightVolumeWidth() const;
	
	float GetLightVolumeHeight() const;

	const D3DXMATRIX &GetViewProjectionMatrix() const;

private:
	LightProperties m_lightProperties;
	LightType m_type;
	float m_zNear;
	float m_zFar;

	float m_dirLightWidth;
	float m_dirLightHeight;

	bool m_isDynamic;			//true sii la luz debe desplazarse junto con la cámara al utilizar el teclado

	bool m_update;				//true sii debemos actualizar esta luz en los shaders en el frame actual como consecuencia de haber cambiado su estado de encendido/apagado

	D3DXMATRIX m_viewMatrix;
	D3DXMATRIX m_projMatrix;
	D3DXMATRIX m_viewProjMatrix;
};

inline void Light::SetDirection(const D3DXVECTOR3 &dir)
{
	m_lightProperties.dir = dir;

	//actualizar la view matrix
	D3DXMatrixLookAtLH(&m_viewMatrix, &(m_lightProperties.pos), &(m_lightProperties.dir), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	m_viewProjMatrix = m_viewMatrix * m_projMatrix;
}
inline void Light::SetPosition(const D3DXVECTOR3 &pos)
{
	m_lightProperties.pos = pos;

	//actualizar la view matrix
	D3DXMatrixLookAtLH(&m_viewMatrix, &(m_lightProperties.pos), &(m_lightProperties.dir), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	m_viewProjMatrix = m_viewMatrix * m_projMatrix;
}
inline void Light::SetProperties(const LightProperties &properties, const LightType type)		//variables usadas en el shader
{
	m_type = type;

	memcpy_s(&m_lightProperties, sizeof(LightProperties), &properties, sizeof(LightProperties));

	switch(type) 
	{
		case DIRECTIONAL_LIGHT:
			m_lightProperties.type = DIRECTIONAL_LIGHT;	//DIRECTIONAL_LIGHT es 0, POINT 1. Esto debe ser consistente con lo definido en Shaders/base.fx
			break;
		case POINT_LIGHT:
			m_lightProperties.type = POINT_LIGHT;
			break;
		default:
			m_lightProperties.type = DIRECTIONAL_LIGHT;
	}

	//actualizar la view matrix
	D3DXMatrixLookAtLH(&m_viewMatrix, &(m_lightProperties.pos), &(m_lightProperties.dir), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	m_viewProjMatrix = m_viewMatrix * m_projMatrix;
}
inline void Light::SetDynamic(const bool b)
{
	m_isDynamic = b;
}
inline void Light::SetZNear(const float f)
{
	m_zNear = f;

	//D3DXMatrixPerspectiveFovLH( &m_projMatrix, ( float )D3DX_PI * 0.25f, 1, Z_NEAR, Z_FAR);
	D3DXMatrixOrthoLH(&m_projMatrix, m_dirLightWidth, m_dirLightHeight, m_zNear, m_zFar);

	m_viewProjMatrix = m_viewMatrix * m_projMatrix;
}
inline void Light::SetZFar(const float f)
{
	m_zFar = f;

	D3DXMatrixOrthoLH(&m_projMatrix, m_dirLightWidth, m_dirLightHeight, m_zNear, m_zFar);

	m_viewProjMatrix = m_viewMatrix * m_projMatrix;
}
inline void Light::SetDirectionalLightVolume(const float w, const float h)
{
	m_dirLightHeight = h;
	m_dirLightWidth = w;

	D3DXMatrixOrthoLH(&m_projMatrix, m_dirLightWidth, m_dirLightHeight, m_zNear, m_zFar);

	m_viewProjMatrix = m_viewMatrix * m_projMatrix;
}
inline void Light::SetOnOffState()
{
	m_lightProperties.on = (m_lightProperties.on == 1) ? 0 : 1;
	m_update = true;
}
inline void Light::SetOnState()
{
	m_lightProperties.on = 1;
	m_update = true;
}
inline void Light::SetOffState()
{
	m_lightProperties.on = 0;
	m_update = true;
}
inline void Light::OnOffUpdateDone()
{
	m_update = false;
}

inline const D3DXVECTOR3 &Light::GetPosition() const
{
	return m_lightProperties.pos;
}
inline const D3DXVECTOR3 &Light::GetDirection() const
{
	return m_lightProperties.dir;
}
inline LightType Light::GetType() const
{
	return m_type;
}
inline float Light::GetRange() const
{
	return m_lightProperties.range;
}
inline const LightProperties &Light::GetProperties() const
{
	return m_lightProperties;
}
inline float Light::GetShadowMapBias() const
{
	return m_lightProperties.shadowMapBias;
}
inline bool Light::IsOn() const
{
	return (m_lightProperties.on == 1 ? true : false);
}

inline bool Light::IsDynamic() const
{
	return m_isDynamic;
}
inline bool Light::ShouldUpdate() const
{
	return (m_update || m_isDynamic);
}
inline float Light::GetZNear() const
{
	return m_zNear;
}
inline float Light::GetZFar() const
{
	return m_zFar;
}
inline float Light::GetLightVolumeWidth() const
{
	return m_dirLightWidth;
}
inline float Light::GetLightVolumeHeight() const
{
	return m_dirLightHeight;
}

inline const D3DXMATRIX &Light::GetViewProjectionMatrix() const
{
	return m_viewProjMatrix;
}


}

#endif
