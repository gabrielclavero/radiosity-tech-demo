//------------------------------------------------------------------------------------------
// File: Utility.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


#include "Utility.h"

namespace DTFramework
{

void MiscErrorWarning(UINT errorCode, LPCWSTR lpcwsFunction)
{
	LPCWSTR lpMsgBuf;
	LPCWSTR titulo = L"Error";

	if(lpcwsFunction)
		titulo = lpcwsFunction;

	switch(errorCode) 
	{
		case BAD_ALLOC:
			lpMsgBuf = L"Fallo en reserva de memoria.";
			break;
		case MEMCPY:
		case WCSCPYERROR:
			lpMsgBuf = L"Fallo en copiado de memoria.";
			break;
		case WCSCATERROR:
			lpMsgBuf = L"Fallo en concatenado de cadenas.";
			break;
		case DRAWTEXT:
			lpMsgBuf = L"Fallo en ID3DX10::DrawText.";
			break;
		case WCSCAT:
			lpMsgBuf = L"Fallo en concatenado de cadenas.";
			break;
		case MATERIAL_ERROR:
			lpMsgBuf = L"Fallo en constructor de Material.";
			break;
		case NOT_ENOUGH_SPACE:
			lpMsgBuf = L"No hay espacio suficiente en buffer.";
			break;
		case SS_ERROR:
			lpMsgBuf = L"Error en stringstream. Verifique el formato de su archivo.";
			break;
		case INVALID_PARAMETER:
			lpMsgBuf = lpcwsFunction;
			titulo = L"Parametro invalido.";
			break;
		case IFSTREAM_ERROR:
			lpMsgBuf = L"Error leyendo un archivo con ifstream.";
			break;
		case LENGTH_ERROR:
			lpMsgBuf = L"Error en vector::Reserve. Excede longitud maxima para vector.";
			break;
		case MESHFILE_ERROR:
			lpMsgBuf = L"Formato .obj no válido.";
			break;
		case MATERIALFILE_ERROR:
			lpMsgBuf = L"Formato .mtl no válido.";
			break;
		case MISCSCENEFILE_ERROR:
			lpMsgBuf = L"Archivo de escena incorrecto.";
			break;
		case NO_SSE_SUPPORT:
			lpMsgBuf = L"Biblioteca DirectXMath no está soportada en la actual plataforma.";
			break;
		case EXCEEDED_MAX_DISPATCH_DIMENSION:
			lpMsgBuf = L"Los valores elegidos exceden el máximo permitido para cada dimensión en la llamada a Dispatch.";
			break;
		case PROFFILE_ERROR:
			lpMsgBuf = L"Verifique el formato de su archivo de configuración de profiling.";
			break;
		case WRONG_ENGINE_CONFIG:
			lpMsgBuf = L"Parámetros de creación de Engine incorrectos.";
			break;
		case BAD_ALIGNED_ALLOC:
			lpMsgBuf = L"Fallo en reserva de memoria con alineamiento.";
			break;
		case INVALID_FUNCTION_CALL:
			lpMsgBuf = L"Llamada a función no válida en contexto actual o parámetros incorrectos.";
			break;
		default:
			lpMsgBuf = L"Error desconocido";
	}

	MessageBox(NULL, lpMsgBuf, titulo, MB_OK);
}

void ErrorWarning(LPCWSTR lpcwsFunction)
{
	//Obtener mensaje de error del último código de error
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	              NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);

	//Imprimir mensaje de error y terminar el proceso
	LPCWSTR lpDisplayBuf = new (std::nothrow) WCHAR[wcslen(lpcwsFunction)+ wcslen((LPCWSTR)lpMsgBuf) + 40];
	if(lpDisplayBuf == NULL) { MessageBox(NULL, NULL, L"Error", MB_OK); return; }
	wsprintf((LPWSTR)lpDisplayBuf, L"%s ha fallado con error %d: %s\0", lpcwsFunction, dw, lpMsgBuf);
	
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, L"Error", MB_OK);

	SAFE_DELETE_ARRAY(lpDisplayBuf);
}

//errores de funciones COM
void COMErrorWarning(HRESULT hr, LPCWSTR lpcwsFunction)
{
	LPCWSTR lpMsgBuf;
	switch(hr) 
	{
		case E_ABORT:
			lpMsgBuf = L"La operación fue abortada por un error no especificado";
			break;
		case E_ACCESSDENIED:
			lpMsgBuf = L"Error de acceso denegado";
			break;
		case E_FAIL:
			lpMsgBuf = L"Falla no especificada";
			break;
		case E_HANDLE:
			lpMsgBuf = L"Uso de handle no válido";
			break;
		case E_INVALIDARG:
			lpMsgBuf = L"Uno o mas argumentos no es valido";
			break;
		case E_NOINTERFACE:
			lpMsgBuf = L"El método QueryInterface no reconoció la interfaz requerida. La interfaz no es soportada";
			break;
		case E_NOTIMPL:
			lpMsgBuf = L"El método no está implementado";
			break;
		case E_OUTOFMEMORY:
			lpMsgBuf = L"El método ha fallado en reservar memoria";
			break;
		case E_PENDING:
			lpMsgBuf = L"Los datos necesarios para completar la operación no estan aun disponibles";
			break;
		case E_POINTER:
			lpMsgBuf = L"Un puntero no válido ha sido usado";
			break;
		case E_UNEXPECTED:
			lpMsgBuf = L"Un fallo imprevisto ha ocurrido";
			break;
		default:
			lpMsgBuf = L"Error desconocido";
	}

	LPCWSTR lpDisplayBuf = new (std::nothrow) WCHAR[wcslen(lpMsgBuf)+wcslen(lpcwsFunction)+50];
	if(lpDisplayBuf == NULL) { MessageBox(NULL, NULL, L"Error COM", MB_OK); return; }
	wsprintf((LPWSTR)lpDisplayBuf, L"La función %s ha fallado. Mensaje de error: %s", lpcwsFunction, lpMsgBuf);

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, L"Error COM", MB_OK);

	SAFE_DELETE_ARRAY(lpDisplayBuf);
}

void DXGI_D3D_ErrorWarning(HRESULT hr, LPCWSTR lpcwsFunction)
{
	LPCWSTR lpMsgBuf;
	switch(hr) 
	{
		case DXGI_ERROR_DEVICE_HUNG:
			lpMsgBuf = L"DXGI_ERROR_DEVICE_HUNG";
			break;
		case DXGI_ERROR_DEVICE_REMOVED:
			lpMsgBuf = L"DXGI_ERROR_DEVICE_REMOVED";
			break;
		case DXGI_ERROR_DEVICE_RESET:
			lpMsgBuf = L"DXGI_ERROR_DEVICE_RESET";
			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			lpMsgBuf = L"DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break;
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
			lpMsgBuf = L"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
			break;
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
			lpMsgBuf = L"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
			break;
		case DXGI_ERROR_INVALID_CALL:
			lpMsgBuf = L"DXGI_ERROR_INVALID_CALL";
			break;
		case DXGI_ERROR_MORE_DATA:
			lpMsgBuf = L"DXGI_ERROR_MORE_DATA";
			break;
		case DXGI_ERROR_NONEXCLUSIVE:
			lpMsgBuf = L"DXGI_ERROR_NONEXCLUSIVE";
			break;
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			lpMsgBuf = L"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
			break;
		case DXGI_ERROR_NOT_FOUND:
			lpMsgBuf = L"DXGI_ERROR_NOT_FOUND";
			break;
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
			lpMsgBuf = L"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
			break;
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:
			lpMsgBuf = L"DXGI_ERROR_REMOTE_OUTOFMEMORY";
			break;
		case DXGI_ERROR_WAS_STILL_DRAWING:
			lpMsgBuf = L"DXGI_ERROR_WAS_STILL_DRAWING";
			break;
		case DXGI_ERROR_UNSUPPORTED:
			lpMsgBuf = L"DXGI_ERROR_UNSUPPORTED";
			break;

		case D3D10_ERROR_FILE_NOT_FOUND:
		case D3D11_ERROR_FILE_NOT_FOUND:
			lpMsgBuf = L"ERROR_FILE_NOT_FOUND";
			break;
		case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			lpMsgBuf = L"ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break;
		case D3DERR_INVALIDCALL:
			lpMsgBuf = L"D3DERR_INVALIDCALL";
			break;
		case D3DERR_WASSTILLDRAWING:
			lpMsgBuf = L"D3DERR_WASSTILLDRAWING";
			break;
		case E_FAIL:
			lpMsgBuf = L"E_FAIL";
			break;
		case E_INVALIDARG:
			lpMsgBuf = L"E_INVALIDARG";
			break;
		case E_OUTOFMEMORY:
			lpMsgBuf = L"E_OUTOFMEMORY";
			break;
		default:
			lpMsgBuf = L"Error desconocido";
	}

	LPCWSTR lpDisplayBuf = new (std::nothrow) WCHAR[wcslen(lpMsgBuf)+wcslen(lpcwsFunction)+50];
	if(lpDisplayBuf == NULL) { MessageBox(NULL, NULL, L"Error DXGI - D3D", MB_OK); return; } 
	wsprintf((LPWSTR)lpDisplayBuf, L"La función %s ha fallado. Mensaje de error: %s", lpcwsFunction, lpMsgBuf);

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, L"Error DXGI - D3D", MB_OK);

	SAFE_DELETE_ARRAY(lpDisplayBuf);
}

}