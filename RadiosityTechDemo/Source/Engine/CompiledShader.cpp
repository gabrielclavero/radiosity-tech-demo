//------------------------------------------------------------------------------------------
// File: CompiledShader.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "CompiledShader.h"

namespace DTFramework
{

CompiledShader::CompiledShader(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_effect(0), m_vertexShader(0), m_pixelShader(0), m_computeShader(0)
{

}

CompiledShader::~CompiledShader()
{
	SAFE_RELEASE(m_effect);
	SAFE_RELEASE(m_vertexShader);
	SAFE_RELEASE(m_pixelShader);
	SAFE_RELEASE(m_computeShader);
}

HRESULT CompiledShader::LoadPrecompiledShader(const wstring &fileName)
{
	SAFE_RELEASE(m_effect);

	HRESULT hr = S_OK;

	char *effectBuffer = NULL;
	wstring fullpath = SHADERS_DIRECTORY + fileName;
	ifstream is;

	try 
	{
		is.open(fullpath, std::ios::binary);
		if(is.fail()) throw 'e';

		is.seekg(0, std::ios_base::end);
		if(is.fail()) throw 'e';

		//obtengo la cantidad de bytes en el archivo compilado de efecto
		std::streampos pos = is.tellg();			
		if(pos <= 0) throw 'e';

		//muevo la posición de lectura al comienzo
		is.seekg(0, std::ios_base::beg);			
		if(is.fail()) throw 'e';

		//leer todos los bytes del archivo y copiarlos al buffer effectBuffer
		effectBuffer = new char[ static_cast<UINT> (pos) ];	
		is.read(effectBuffer, pos);
		if(is.eof() || is.fail()) throw 'e';
	
		//crear el effect
		hr = m_d3dManager.CreateEffectFromMemory((void *) (effectBuffer), (SIZE_T) pos, 0, &m_effect);
	} 
	catch (char &) 
	{
		MiscErrorWarning(IFSTREAM_ERROR);
		hr = E_FAIL;
	}
	catch(std::bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		hr = E_FAIL;
	}

	SAFE_DELETE_ARRAY(effectBuffer);
	if(is.is_open())
		is.close();

	return hr;
}

ID3DBlob *CompiledShader::ObtainBlob(const wstring &fileName, const string &functionName, const char * const profile, const D3D_SHADER_MACRO * const defines)
{
	_ASSERT(profile);

	if(!profile) {
		MiscErrorWarning(INVALID_PARAMETER, L"CompiledShader::ObtainBlob");
		return NULL;
	}

	HRESULT hr;

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorMessage = nullptr;
	
	#if defined(DEBUG)
		const UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		const UINT flags = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ENABLE_STRICTNESS;
	#endif

	//en fileName solo esta el nombre del archivo, pero no la ruta. La ruta es SHADERS_DIRECTORY
	wstring fullpath = SHADERS_DIRECTORY + fileName;
	
	//segundo parámetro es un arreglo que define valores para constantes que se encuentran en el código HLSL. Como el preprocesador de C.
	//Asegurarse de incluir d3dcompiler.lib o que d3dcompiler_44.dll (o superior) pueda accederse desde el ejecutable para usar esta función
	if(FAILED(hr = D3DCompileFromFile(fullpath.c_str(), defines, NULL, functionName.c_str(), profile, flags, 0, &shaderBlob, &errorMessage) )) 
	{
		OutputShaderErrorMessage(errorMessage, fileName);
		DXGI_D3D_ErrorWarning(hr, L"D3DCompileFromFile");

		SAFE_RELEASE(shaderBlob);
		SAFE_RELEASE(errorMessage);
		return NULL;
	}

	SAFE_RELEASE(errorMessage);

	return shaderBlob;
}

HRESULT CompiledShader::CompileVertexShader(const wstring &fileName, const string &functionName)
{
	SAFE_RELEASE(m_vertexShader);

	HRESULT hr = S_OK;

	ID3DBlob *shaderBlob = ObtainBlob(fileName, functionName, "vs_5_0");
	if(shaderBlob == NULL) return E_FAIL;

	hr = m_d3dManager.CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &m_vertexShader);

	SAFE_RELEASE(shaderBlob);

	return hr;
}

HRESULT CompiledShader::CompilePixelShader(const wstring &fileName, const string &functionName)
{
	SAFE_RELEASE(m_pixelShader);

	HRESULT hr = S_OK;

	ID3DBlob *shaderBlob = ObtainBlob(fileName, functionName, "ps_5_0");
	if(shaderBlob == NULL) return E_FAIL;

	hr = m_d3dManager.CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &m_pixelShader);

	SAFE_RELEASE(shaderBlob);

	return hr;
}

HRESULT CompiledShader::CompileComputeShader(const wstring &fileName, const string &functionName, const D3D_SHADER_MACRO * const defines)
{
	SAFE_RELEASE(m_computeShader);

	HRESULT hr = S_OK;

	ID3DBlob *shaderBlob = ObtainBlob(fileName, functionName, "cs_5_0", defines);
	if(shaderBlob == NULL) return E_FAIL;

	hr = m_d3dManager.CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &m_computeShader);

	SAFE_RELEASE(shaderBlob);

	return hr;
}

void CompiledShader::OutputShaderErrorMessage(ID3DBlob *errorMessage, const wstring &fileName)
{
	_ASSERT(errorMessage);

	if(!errorMessage) return;

	m_compilationErrors.open(COMPILATION_ERRORS_FILE);
	m_compilationErrors << L"File: " << fileName << endl;

	char *compileErrors = NULL;
	compileErrors = static_cast<char *>(errorMessage->GetBufferPointer());

	if(compileErrors) {
		for(UINT i=0; i<errorMessage->GetBufferSize(); ++i)
			m_compilationErrors << compileErrors[i];
	}

	m_compilationErrors.close();
}

}