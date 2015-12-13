//------------------------------------------------------------------------------------------
// File: Engine.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Engine.h"

namespace DTFramework
{

static Engine *g_dtengine=NULL;

LRESULT CALLBACK WindowProc(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) 
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		
		//el resto de los mensajes lo pasamos al manejador en la clase Engine
		default:
			return g_dtengine->MessageHandler(wnd, uMsg, wParam, lParam);
	}

	return 0;
}

Engine::Engine()
: m_inputHandler(0), m_scene(0), m_renderer(0), m_gi(0), m_window(0), m_deactive(false), m_exit(false), m_ready(false)
{
	g_dtengine = this;
}

Engine::~Engine()
{
	SAFE_DELETE(m_inputHandler);
	SAFE_DELETE(m_scene);
	SAFE_DELETE(m_gi);
	SAFE_DELETE(m_renderer);

	ShowCursor(true);
}

HRESULT Engine::Init(const EngineConfig * const config)
{
	_ASSERT(!m_ready && config);

	if(m_ready || !config) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Engine::Init");
		return E_FAIL;
	}

	if(config->scale <= 0 || config->windowMinHeight <= 0 || config->windowMinWidth <= 0 || config->totalBackBuffers <= 0) {
		MiscErrorWarning(WRONG_ENGINE_CONFIG, L"Engine::Init");
		return E_FAIL;
	}

	HRESULT hr;
	std::ofstream logfile("log.log");
	Timer tmpTimer(m_d3dManager);
	tmpTimer.Start();

	WCHAR *wtmp = NULL;
	try 
	{
		//guardamos el directorio inicial actual del proceso (cada proceso tiene uno) para volver a setearlo si es que necesitamos cambiarlo
		DWORD ret = GetCurrentDirectoryW(0, NULL);
		if(ret == 0) {
			ErrorWarning(L"GetCurrentDirectory");
			return E_FAIL;
		} 
		wtmp = new WCHAR[ret];
		ret = GetCurrentDirectoryW(ret, wtmp);
		if(ret == 0) {
			ErrorWarning(L"GetCurrentDirectory");
			SAFE_DELETE_ARRAY(wtmp);
			return E_FAIL;
		} 
		m_currentDir = wstring(wtmp);
		SAFE_DELETE_ARRAY(wtmp);

		//copiamos parámetros de configuración	
		if(memcpy_s(&m_config, sizeof(EngineConfig), config, sizeof(EngineConfig)) != 0) {
			MiscErrorWarning(MEMCPY);
			return E_FAIL;
		}

		//Inicializamos la biblioteca COM para su uso en este hilo
		if(FAILED(hr=CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY))) {
			COMErrorWarning(hr, L"CoInitializeEx");
			return E_FAIL;
		}

		//Mostrar ventana de configuración
		if( m_settingsDialog.ShowConfigurationDialog(m_config.pixelFormat, m_currentDir, m_config.windowMinWidth, m_config.windowMinHeight) != IDC_OK )
			return E_FAIL;
	
		//seteo el directorio actual del proceso al que teniamos al principio pues en SettingsDialog lo cambiamos
		if(!SetCurrentDirectory(m_currentDir.c_str())) {
			ErrorWarning(L"SetCurrentDirectory");
			return E_FAIL;
		}

		//preparar ventana y mostrarla
		if(FAILED( hr = PrepareWindow() )) return hr;
		if(m_window == NULL) return E_FAIL;
		ShowWindow(m_window, SW_NORMAL);

		//d3d devices manager. Encargado de inicializar direct3d y sus funcionalidades
		if(FAILED(hr = m_d3dManager.Init(m_settingsDialog.GetSelectedDisplayMode(), m_settingsDialog.IsWindowed(), m_settingsDialog.IsVsync(), 
		                                 m_settingsDialog.IsAAEnabled(), m_window, m_config.totalBackBuffers) )) return hr;

		//escena
		m_scene = new Scene(m_d3dManager);

		if(FAILED( hr = m_scene->Init(m_settingsDialog.GetSceneFileName(), &m_camera, &m_light) )) return hr;

		//Renderer object, que se encargará de todas las funcionalidades referidas a la renderización
		m_renderer = new Renderer(m_d3dManager);

		//global illuminaticon con radiosity
		if(m_settingsDialog.IsGIEnabled())
		{
			
			if(m_settingsDialog.IsCpuGIEnabled()) {
				m_gi = new CPURadiosity(m_d3dManager, m_settingsDialog.IsExportHemicubesEnabled(), m_settingsDialog.IsProfilingEnabled(),
				                        m_settingsDialog.GetVerticesBakedPerDispatch(), m_settingsDialog.GetNumBounces() );
			} else {
				m_gi = new GPURadiosity(m_d3dManager, m_settingsDialog.IsExportHemicubesEnabled(), m_settingsDialog.IsProfilingEnabled(),
				                        m_settingsDialog.GetVerticesBakedPerDispatch(), m_settingsDialog.GetVerticesBakedPerDispatch2(), m_settingsDialog.GetNumBounces() );
			}
			
			if(FAILED( hr = m_renderer->Init(m_light.GetType(), m_scene->GetShadowMapsSize(), m_gi->GetHemicubeFaceSize() ))) return hr;
			

			if(FAILED( hr = m_gi->Init( ) )) return hr;
			
			tmpTimer.UpdateForGPU();

			if(FAILED( hr = m_gi->ComputeGIDataForScene(*m_renderer, *m_scene, m_light) )) return hr;

			tmpTimer.UpdateForGPU();
			double loggedTime = tmpTimer.GetTimeElapsed();
			logfile << loggedTime;
			if(logfile.is_open())
				logfile.close();
		} else {
			if(FAILED( hr = m_renderer->Init( m_light.GetType(), m_scene->GetShadowMapsSize() ))) return hr;
		}

		m_config.scale = m_scene->GetScale();

		//input handler
		const UINT width = m_settingsDialog.GetSelectedDisplayMode().Width;
		const UINT height = m_settingsDialog.GetSelectedDisplayMode().Height;
		m_inputHandler = new InputHandler(m_window, width, height, m_config.scale);

		ShowCursor(false);
	} 
	catch (std::bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		return E_FAIL;
	}

	m_ready = true;

	return S_OK;
}

//main loop
void Engine::Run()
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Engine::Run");
		return;
	}

	MSG msg;

	ZeroMemory(&msg, sizeof(MSG));
	while(msg.message != WM_QUIT) 
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
		else if( !m_deactive ) 
		{
			if( m_exit ) 
			{
				m_d3dManager.PrepareForExit();
				PostQuitMessage(0);
			}

			if( FAILED( ProcessFrame() ) )
				PostQuitMessage(0);
		}
	}
}

HRESULT Engine::ProcessFrame()
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Engine::ProcessFrame");
		return E_FAIL;
	}

	return m_renderer->ProcessFrame(*m_scene, m_camera, m_light, m_settingsDialog.IsGIEnabled() ? m_gi->GetGIData() : NULL);
}

LRESULT CALLBACK Engine::MessageHandler(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool w=false, a=false, s=false, d=false;		//w es true si w está presionado. Igual para los otros.

	switch(uMsg) 
	{
		case WM_ACTIVATEAPP:
			m_deactive = (!wParam);
			break;

		case WM_CLOSE:
			if(m_ready)
				m_exit = true;
			else 
				PostQuitMessage(0);
			break;

		case WM_KEYDOWN:
			if(wParam == L'w' || wParam == L'W')
				w = true;
			else if(wParam == L'a' || wParam == L'A')
				a = true;
			else if(wParam == L's' || wParam == L'S')
				s = true;
			else if(wParam == L'd' || wParam == L'D')
				d = true;
			break;

		case WM_KEYUP:
			if(wParam == L'w' || wParam == L'W')
				w = false;
			else if(wParam == L'a' || wParam == L'A')
				a = false;
			else if(wParam == L's' || wParam == L'S')
				s = false;
			else if(wParam == L'd' || wParam == L'D')
				d = false;
			break;


		case WM_CHAR:
			if(m_ready) 
			{
				if(wParam == VK_ESCAPE)
					m_exit = true;
				else if(wParam == L'x' || wParam == L'X')
					m_renderer->TurnOnOffHUD();
				else if(wParam == L'z' || wParam == L'Z')
					m_renderer->TurnOnOffGI();

				else
					m_inputHandler->HandleKeyInput(m_camera, m_light, wParam, w, a, s, d);
			}
			break;

		case WM_MOUSEMOVE:
			if(m_ready)
				m_inputHandler->HandleMouseInput(m_camera, m_light, lParam);
			break;

		case WM_WINDOWPOSCHANGING:
			if(m_ready)
				m_inputHandler->SetMiddlePoint();
			break;

		//el resto de los mensajes lo pasamos al manejador por defecto
		default:
			return DefWindowProc(wnd, uMsg, wParam, lParam);
	}
	return 0;
}

HRESULT Engine::PrepareWindow()
{
	m_window = NULL;

	WNDCLASSEX wcex;
	wcex.cbSize        = sizeof( WNDCLASSEX );
	wcex.style         = CS_CLASSDC;
	wcex.lpfnWndProc   = WindowProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = m_config.hInstance;
	wcex.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
	wcex.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName  = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm       = LoadIcon( NULL, IDI_APPLICATION );

	if(RegisterClassEx( &wcex ) == 0) {
		ErrorWarning(L"RegisterClassEx");
		return E_FAIL;
	}
	
	if((m_window = CreateWindow(L"WindowClass", m_config.title, m_settingsDialog.IsWindowed() ? WS_OVERLAPPEDWINDOW : WS_POPUP, 
		0, 0, m_settingsDialog.GetSelectedDisplayMode().Width, m_settingsDialog.GetSelectedDisplayMode().Height, NULL, NULL, m_config.hInstance, NULL)) == NULL) 
	{
		ErrorWarning(L"CreateWindow");
		return E_FAIL;
	}

	return S_OK;
}

}