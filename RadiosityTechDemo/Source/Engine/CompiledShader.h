//------------------------------------------------------------------------------------------
// File: CompiledShader.h
//
// Esta clase es responsable de cargar un archivo de un shader compilado (extensión .fxo)
// o de un shader en lenguaje HLSL (extensión .fx o .hlsl) 
// En el caso de los shaders compilados se carga todo el archivo.
// En el segundo caso se compila alguna de las funciones (vertex, pixel
// o compute shader) del archivo y se la almacena en la interfaz correspondiente.
// Los shaders deben estar en el directorio SHADERS_DIRECTORY.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef COMPILED_SHADER_H
#define COMPILED_SHADER_H

#include "d3dx11effect.h"
#include "Utility.h"
#include "D3DDevicesManager.h"
#include <D3Dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "Effects11_debug.lib")
#else
#pragma comment(lib, "Effects11.lib")
#endif

using std::wstring;
using std::ifstream;
using std::wofstream;

using std::string;
using std::endl;

namespace DTFramework
{

class CompiledShader
{
public:
	CompiledShader(const D3DDevicesManager &d3d);
	~CompiledShader();

	HRESULT LoadPrecompiledShader(const wstring &fileName);

	HRESULT CompileVertexShader(const wstring &fileName, const string &functionName);
	HRESULT CompilePixelShader(const wstring &fileName, const string &functionName);
	HRESULT CompileComputeShader(const wstring &fileName, const string &functionName, const D3D_SHADER_MACRO * const defines = NULL);
	
	ID3DX11Effect *GetEffect() const;
	ID3D11VertexShader *GetVertexShader() const;
	ID3D11PixelShader *GetPixelShader() const;
	ID3D11ComputeShader *GetComputeShader() const;

private:
	void OutputShaderErrorMessage(ID3DBlob *errorMessage, const wstring &fileName);
	ID3DBlob *ObtainBlob(const wstring &fileName, const string &functionName, const char * const profile, const D3D_SHADER_MACRO * const defines = NULL);

private:
	const D3DDevicesManager &m_d3dManager;

	ID3DX11Effect *m_effect;

	ID3D11VertexShader *m_vertexShader;
	ID3D11PixelShader *m_pixelShader;
	ID3D11ComputeShader *m_computeShader;

	wofstream m_compilationErrors;
};

inline ID3DX11Effect *CompiledShader::GetEffect() const
{
	return m_effect;
}

inline ID3D11VertexShader *CompiledShader::GetVertexShader() const
{
	return m_vertexShader;
}
inline ID3D11PixelShader *CompiledShader::GetPixelShader() const
{
	return m_pixelShader;
}
inline ID3D11ComputeShader *CompiledShader::GetComputeShader() const
{
	return m_computeShader;
}

}

#endif