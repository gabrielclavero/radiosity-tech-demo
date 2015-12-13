//------------------------------------------------------------------------------------------
// File: InputHandler.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "InputHandler.h"

namespace DTFramework
{

InputHandler::InputHandler(const HWND wHandler, const UINT width, const UINT height, const float scale)
: WINDOW_WIDTH(width), WINDOW_HEIGHT(height), WINDOW_HANDLER(wHandler), 
 m_xPos(WINDOW_WIDTH/2), m_yPos(WINDOW_HEIGHT/2), m_scale(scale), m_flagMoved(false)
{
	SetCursorPos(WINDOW_WIDTH/2, WINDOW_HEIGHT/2);
	SetMiddlePoint();
}

void InputHandler::HandleKeyInput(Camera &camera, Light &light, WPARAM wParam, const bool W, const bool A, const bool S, const bool D)
{	
	//1. LUCES

	//tecla f: setear la luz activa como dinámica o estática
	if(wParam == L'f' || wParam == L'F') 
	{
		light.SetDynamic(!light.IsDynamic());
		light.SetPosition(camera.GetCameraPosition());
		light.SetDirection(camera.GetCameraRotation());
	}

	//tecla g: apagar o encender la luz activa
	if(wParam == L'g' || wParam == L'G') 
	{
		light.SetOnOffState();
	}

	//2. DESPLAZAMIENTO CON WASD
	bool updateLightAndCamera = false;
	D3DXVECTOR3 Eye;
	D3DXVECTOR3 At;
	if( ( (W && !S) || (!W && S) ) && !D && !A)					//teclas w s: mover la camara hacia atrás o adelante
	{
		D3DXVECTOR3 direccion = camera.GetCameraRotation() - camera.GetCameraPosition();
		D3DXVec3Normalize(&direccion, &direccion);
		Eye = camera.GetCameraPosition();
		At = camera.GetCameraRotation();

		if(W) 
		{
			Eye += (direccion * m_scale);
			At = Eye + direccion * m_scale * REGULADOR;
		}
		
		if(S) 
		{
			Eye -= (direccion * m_scale);
			At = Eye + direccion * m_scale * REGULADOR;
		}

		updateLightAndCamera = true;
	}
	else if( (W && A && !S && !D) || (S && D && !W && !A) )		//diagonal "w a" o diagonal "s d". Notar que "s d" es opuesta a "w a"
	{
		Eye = camera.GetCameraPosition();
		At = camera.GetCameraRotation();
		D3DXVECTOR3 aux = camera.GetCameraRotation();
		aux.y = Eye.y;

		D3DXVECTOR3 P = aux - Eye;
		D3DXVECTOR3 Q = P;
		D3DXVec3Cross(&P, &P, &(camera.GetUpVector()));		//vector normal al plano formado por los vectores Up y P

		D3DXVec3Normalize(&Q, &Q);
		D3DXVec3Normalize(&P, &P);

		P = Eye + P;
		Q = Eye + Q;

		D3DXVECTOR3 diagonal = ( ( (Q - P) / 2 ) + P ) - Eye;

		if(W) //diagonal w a
		{		
			Eye += (diagonal * m_scale);
			At += (diagonal * m_scale);
		} 
		else //diagonal s d
		{
			Eye -= (diagonal * m_scale);
			At -= (diagonal * m_scale);
		}

		updateLightAndCamera = true;
	}
	else if( (W && D && !S && !A) || (S && A && !W && !D) )		//diagonal "w d" o diagonal "s a". Notar que "s a" es opuesta a "w d"
	{
		Eye = camera.GetCameraPosition();
		At = camera.GetCameraRotation();
		D3DXVECTOR3 aux = camera.GetCameraRotation();
		aux.y = Eye.y;

		D3DXVECTOR3 P = aux - Eye;
		D3DXVECTOR3 Q = P;
		D3DXVec3Cross(&P, &P, &(camera.GetUpVector()));

		D3DXVec3Normalize(&Q, &Q);
		D3DXVec3Normalize(&P, &P);

		P = Eye + P;
		Q = Eye + Q;

		D3DXVECTOR3 diagonal = (Q - P) / 2;

		if(W) //diagonal w d
		{		
			Eye += (diagonal * m_scale);
			At += (diagonal * m_scale);
		} 
		else // diagonal s a
		{	
			Eye -= (diagonal * m_scale);
			At -= (diagonal * m_scale);
		}

		updateLightAndCamera = true;
	}
	else if( ( (A && !D) || (D && !A) ) && !W && !S)			//teclas a d: mover la camara a izquierda o derecha
	{
		Eye = camera.GetCameraPosition();
		At = camera.GetCameraRotation();
		D3DXVECTOR3 aux = camera.GetCameraRotation();
		aux.y = Eye.y;
		D3DXVECTOR3 direccion = aux - Eye;
		D3DXVec3Cross(&direccion, &direccion, &(camera.GetUpVector()));
		D3DXVec3Normalize(&direccion, &direccion);

		if(A) 
		{
			Eye += (direccion * m_scale);
			At += (direccion * m_scale);
		}
		if(D) 
		{
			Eye -= (direccion * m_scale);
			At -= (direccion * m_scale);
		}

		updateLightAndCamera = true;
	}

	if(updateLightAndCamera) 
	{
		//camara
		camera.SetEyeAndAtVectors(Eye, At);
		//luz
		if(light.IsDynamic())
		{
			light.SetPosition(Eye);
			light.SetDirection(At);
		}
	}
}

void InputHandler::HandleMouseInput(Camera &camera, Light &light, LPARAM lParam)
{
	int newxPos = GET_X_LPARAM(lParam);
	int newyPos = GET_Y_LPARAM(lParam);

	if(m_flagMoved) //si hemos centrado el cursor del mouse entonces actualizamos la nueva posicion del cursor pero no movemos la camara porq ha sido un movimiento artificial, no del usuario
	{	
		m_flagMoved = false;
		m_xPos = newxPos; m_yPos = newyPos;
		return;
	}


	D3DXVECTOR3 direccion = camera.GetCameraRotation() - camera.GetCameraPosition();
	D3DXVec3Cross(&direccion, &direccion, &(camera.GetUpVector()));	//obtener vector perpendicular al eje de visión (va a dar la direccion horizontal)
	D3DXVec3Normalize(&direccion, &direccion);

	D3DXVECTOR3 displacement_vector = D3DXVECTOR3( static_cast<float> (newxPos - m_xPos) * direccion.x,  static_cast<float> (newyPos - m_yPos),  static_cast<float> (newxPos - m_xPos) * direccion.z);

	//actualizar el vector At en base a cómo se movió el mouse
	D3DXVECTOR3 At = camera.GetCameraRotation();
	At.y += (m_yPos-newyPos) * m_scale;
	int fuerza = abs(m_xPos - newxPos);
	if(m_xPos - newxPos > 0) //se movió el mouse a la derecha
	{	
		At.x += direccion.x * fuerza * m_scale;
		At.z += direccion.z * fuerza * m_scale;
	} 
	else //se movió a la izquierda
	{ 
		At.x -= direccion.x * fuerza * m_scale;
		At.z -= direccion.z * fuerza * m_scale;
	}

	camera.SetAtVector(At);

	m_xPos = newxPos; m_yPos = newyPos;

	if(newxPos >= (WINDOW_WIDTH / 2) + 10 || newxPos <= (WINDOW_WIDTH / 2) - 10 || newyPos >= (WINDOW_HEIGHT / 2) + 10 || newyPos <= (WINDOW_HEIGHT / 2) - 10) 
	{
		m_flagMoved = true;
		SetCursorPos(m_xMiddle, m_yMiddle);
		m_xPos = m_xMiddle;
		m_yPos = m_yMiddle;
	}

	//luz
	if(light.IsDynamic()) 
		light.SetDirection(At);
}

//el punto medio de la ventana cambia si movemos la posición de la misma en el escritorio
void InputHandler::SetMiddlePoint()
{
	RECT rect1, rect2;
	GetWindowRect(WINDOW_HANDLER, &rect1);
	GetClientRect(WINDOW_HANDLER, &rect2);
	m_xMiddle = rect1.left+rect2.right/2; 
	m_yMiddle = rect1.top+rect2.bottom/2;
}

}