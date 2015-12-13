#include "Engine\Engine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	DTFramework::EngineConfig config;
	config.hInstance = hInstance;
	config.title = L"Radiosity in Direct3D 11 - Tech Demo";
	config.totalBackBuffers = 1;
	config.windowMinWidth = 1366;
	config.windowMinHeight = 768;
	
	DTFramework::Engine engine;
	if(FAILED(engine.Init(&config))) return -1;

	engine.Run();

	return 0;
}