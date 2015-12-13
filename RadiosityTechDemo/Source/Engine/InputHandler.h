//------------------------------------------------------------------------------------------
// File: InputHandler.h
//
// Manejamos los inputs del usuario (teclado y mouse) que requieran de algún 
// procesamiento más complicado.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <windows.h>
#include <windowsx.h>

#include <d3d10.h>
#include <dxgi.h>
#include <d3dx10math.h>

#include <vector>

#include "Light.h"
#include "Camera.h"

using std::vector;

namespace DTFramework
{

class InputHandler
{
public:
	InputHandler(const HWND, const UINT, const UINT, const float);

	void HandleKeyInput(Camera &camera, Light &light, WPARAM wParam, const bool w, const bool a, const bool s, const bool d);
	void HandleMouseInput(Camera &camera, Light &light, LPARAM lParam);
	void SetMiddlePoint();

private:
	const int WINDOW_WIDTH;
	const int WINDOW_HEIGHT;
	const HWND WINDOW_HANDLER;
	static const int REGULADOR = 100;	//compensar movimientos bruscos del mouse

	int m_xPos;             //coordenada x del cursor, se actualiza cada vez que movemos el mouse, asi que antes de actualizarlo tenemos la posición anterior
	int m_yPos;            //coordenada y del cursor	
	
	int m_xMiddle;        //punto medio de la ventana donde estamos corriendo
	int m_yMiddle;

	float m_scale;        //escala a la que opera la escena. La velocidad de desplazamiento se efectua en unidades de escala

	bool m_flagMoved;    //flag que se activa si hemos centrado el cursor del mouse en la ventana (para que éste no se salga de la misma)
};

}

#endif