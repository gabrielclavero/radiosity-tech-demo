//------------------------------------------------------------------------------------------
// File: Engine.h
//
// Un único punto de contacto para inicializar y utilizar todas las funcionalidades
// de este software. Todos los principales objetos y el event loop se encuentran aquí.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef ENGINE_H
#define ENGINE_H

//con esto reducimos el tamaño de cabeceras de Win32 y aceleramos la compilación
#define WIN32_LEAN_AND_MEAN	

#include <objbase.h>

#include "Utility.h"
#include "SettingsDialog.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "Camera.h"
#include "Light.h"
#include "GPURadiosity.h"
#include "CPURadiosity.h"
#include "InputLayouts.h"
#include "Timer.h"

using std::wstring;
using std::vector;

namespace DTFramework
{

//utilizado para configurar algunas opciones de creación del engine
struct EngineConfig
{
	HINSTANCE hInstance;
	WCHAR *title;

	float scale;
	UINT totalBackBuffers;
	UINT windowMinWidth;
	UINT windowMinHeight;

	DXGI_FORMAT pixelFormat;     //formato del pixel para el back buffer. Por defecto es color de 24 bits (8 bits por canal)
	                            //procurar elegir un formato soportado por monitores

	static const UINT WINDOW_WIDTH = 1280;
	static const UINT WINDOW_HEIGHT = 720;


	EngineConfig(const UINT n_buffers = 2, const UINT width = WINDOW_WIDTH, 
	             const UINT height = WINDOW_HEIGHT, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
	: hInstance(0), title(L"Engine11"), scale(1.0f), totalBackBuffers(n_buffers), 
	  windowMinWidth(width), windowMinHeight(height), pixelFormat(format)
	{

	}
};

class Engine
{
public:
	Engine();
	virtual ~Engine();

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init(const EngineConfig * const config = NULL);

	virtual void Run();

	virtual LRESULT CALLBACK MessageHandler(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	HRESULT PrepareWindow();
	virtual HRESULT ProcessFrame();

protected:
	SettingsDialog m_settingsDialog;

	EngineConfig m_config;

	wstring m_currentDir;

	D3DDevicesManager m_d3dManager;

	InputHandler *m_inputHandler;
	Scene *m_scene;
	Renderer *m_renderer;
	Radiosity *m_gi;

	Camera m_camera;
	Light m_light;

	HWND m_window;

	bool m_deactive;	//el procesado de cada frame se desactiva cuando esta variable esté en true. Esto sucede cuando la ventana de la app no sea la ventana activa
	bool m_exit;		//true cuando la aplicación este por cerrar con ESC o ALT+F4 para, entre otras cosas, switchear a modo window antes de destruir el swap chain
	
	bool m_ready;
};

static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

}

#endif