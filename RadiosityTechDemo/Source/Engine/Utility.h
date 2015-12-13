//------------------------------------------------------------------------------------------
// File: Utility.h
//
// Constantes generales, macros y funciones que imprimen los mensajes de error.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef UTILITY_H
#define UTILITY_H

#define NOMINMAX

#include <Windows.h>
#include <vector>
#include <new>
#include <fstream>
#include <string>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dx11.h>
#include <d3dx10math.h>

#include "Geometry.h"


//MACROS DE MANEJO DE MEMORIA
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

//DEBUG
//#define DEBUG
//#define DEBUG_TEXTURES
//#define DEBUG_GI_GRAPHICS_DATA


//DIRECTORIOS
#define SEARCH_SCENE_DIRECTORY  L"\\Assets\\Scenes\\*.txt"
#define SCENES_DIRECTORY        L"Assets/Scenes/"
#define MTLS_DIRECTORY          L"Assets/Meshes/"
#define MESHES_DIRECTORY        L"Assets/Meshes/"
#define TEXTURES_DIRECTORY      L"Assets/Textures/"
#define SHADERS_DIRECTORY       L"Assets/Shaders/"

namespace DTFramework
{

namespace
{
	//FILES
	const std::wstring BASE_SHADER_FILE(L"commonMaterialShader.fxo");
	const std::wstring DIR_SHADOW_EFFECT_FILE(L"renderShadow.fxo");
	const std::wstring OMNI_SHADOW_EFFECT_FILE(L"renderOmniShadow.fxo");
	const std::wstring SKY_TEXTURE_EFFECT_FILE (L"skyTexture.fxo");
	const std::wstring SKY_BOX_EFFECT_FILE (L"skyBox.fxo");
	const std::wstring GI_SHADER_FILE (L"hemicubesIntegration.hlsl");
	const std::wstring DEPTH_ONLY_EFFECT_FILE (L"depthOnly.fxo");
	const std::wstring RENDER_TEXTURE_DEBUG_EFFECT_FILE (L"renderTextureDebug.fxo");
	const std::wstring PROFILER_SETTINGS(L"prf_settings.txt");
	const std::wstring PROFILING_FILE(L"profiling.txt");
	const std::wstring COMPILATION_ERRORS_FILE(L"compilation_errors.txt");
}

namespace
{
	//ERROR CODES
	const UINT BAD_ALLOC = 0;
	const UINT MEMCPY = 1;
	const UINT WCSCPYERROR = 2;
	const UINT WCSCATERROR = 3;
	const UINT DRAWTEXT = 4;
	const UINT WCSCAT = 5;
	const UINT MATERIAL_ERROR = 6;
	const UINT NOT_ENOUGH_SPACE = 7;
	const UINT SS_ERROR = 8;
	const UINT INVALID_PARAMETER = 9;
	const UINT IFSTREAM_ERROR = 10;
	const UINT LENGTH_ERROR = 11;
	const UINT MESHFILE_ERROR = 12;
	const UINT MATERIALFILE_ERROR = 13;
	const UINT MISCSCENEFILE_ERROR = 14;
	const UINT NO_SSE_SUPPORT = 15;
	const UINT EXCEEDED_MAX_DISPATCH_DIMENSION = 16;
	const UINT PROFFILE_ERROR = 17;
	const UINT WRONG_ENGINE_CONFIG = 18;
	const UINT BAD_ALIGNED_ALLOC = 19;
	const UINT INVALID_FUNCTION_CALL = 20;
}

//mostrar errores de funciones windows
void ErrorWarning(LPCWSTR lpcwsFunction);

//mostrar errores de funciones COM
void COMErrorWarning(HRESULT hr, LPCWSTR lpszFunction);

//mostrar errores DXGI D3D
void DXGI_D3D_ErrorWarning(HRESULT hr, LPCWSTR lpcwsFunction);

//mostrar errores de otra categoría
void MiscErrorWarning(UINT, LPCWSTR=NULL);

}


#endif UTILITY_H