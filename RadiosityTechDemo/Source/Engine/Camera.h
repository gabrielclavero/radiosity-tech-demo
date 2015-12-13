//------------------------------------------------------------------------------------------
// File: Camera.h
//
// Describe la posición, orientación, up vector, view y projection matrices de una cámara 
// virtual perspective en world space.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------
#ifndef CAMERA_H
#define CAMERA_H

#include <d3dx10math.h>

namespace DTFramework
{

class Camera
{
public:
	Camera()
	{

	}

	const D3DXVECTOR3 &GetCameraPosition() const;
	
	const D3DXVECTOR3 &GetCameraRotation() const;
	
	const D3DXVECTOR3 &GetUpVector() const;
	
	const D3DXMATRIX &GetViewMatrix() const;
	
	const D3DXMATRIX &GetProjectionMatrix() const;
	
	const D3DXMATRIXA16 &GetViewProjectionMatrix() const;
	
	void SetEyeAndAtVectors(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &at);
	void SetEyeVector(const D3DXVECTOR3 &eye);
	void SetAtVector(const D3DXVECTOR3 &at);
	void SetUpVector(const D3DXVECTOR3 &up);

	void SetProjectionMatrix(const float aspect, const float fov, const float z_near, const float z_far);

private:
	D3DXVECTOR3 m_eye;
	D3DXVECTOR3 m_at;
	D3DXVECTOR3 m_up;

	D3DXMATRIX m_viewMatrix;
	D3DXMATRIX m_projectionMatrix;
	D3DXMATRIXA16 m_viewProjectionMatrix;
};

inline const D3DXVECTOR3 &Camera::GetCameraPosition() const
{
	return m_eye;
}
inline const D3DXVECTOR3 &Camera::GetCameraRotation() const
{
	return m_at;
}
inline const D3DXVECTOR3 &Camera::GetUpVector() const
{
	return m_up;
}
inline const D3DXMATRIX &Camera::GetViewMatrix() const
{
	return m_viewMatrix;
}
inline const D3DXMATRIX &Camera::GetProjectionMatrix() const
{
	return m_projectionMatrix;
}
inline const D3DXMATRIXA16 &Camera::GetViewProjectionMatrix() const
{
	return m_viewProjectionMatrix;
}


inline void Camera::SetEyeAndAtVectors(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &at)
{
	m_eye = eye;
	m_at = at;

	//actualizamos la viewmatrix
	D3DXMatrixLookAtLH( &m_viewMatrix, &m_eye, &m_at, &m_up );

	//y la view projection
	m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
}
inline void Camera::SetAtVector(const D3DXVECTOR3 &at)
{
	m_at = at;

	//actualizamos la viewmatrix
	D3DXMatrixLookAtLH( &m_viewMatrix, &m_eye, &m_at, &m_up );

	//y la view projection
	m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
}
inline void Camera::SetEyeVector(const D3DXVECTOR3 &eye)
{
	m_eye = eye;

	//actualizamos la viewmatrix
	D3DXMatrixLookAtLH( &m_viewMatrix, &m_eye, &m_at, &m_up );

	//y la view projection
	m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
}
inline void Camera::SetUpVector(const D3DXVECTOR3 &up)
{
	m_up = up;

	//actualizamos la viewmatrix
	D3DXMatrixLookAtLH( &m_viewMatrix, &m_eye, &m_at, &m_up );

	//y la view projection
	m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
}

inline void Camera::SetProjectionMatrix(const float aspect, const float fov, const float z_near, const float z_far)
{
	// inicializar la matriz de proyección
	D3DXMatrixPerspectiveFovLH( &m_projectionMatrix, fov, aspect, z_near, z_far);
}

}

#endif CAMERA_H