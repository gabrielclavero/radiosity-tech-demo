//------------------------------------------------------------------------------------------
// File: InputLayouts.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "InputLayouts.h"

namespace DTFramework
{

//static member variables inicialización
D3D11_INPUT_ELEMENT_DESC InputLayouts::m_standardLayoutDesc[4] = 
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
UINT InputLayouts::m_standardLayoutNumElements = 4;
D3D11_INPUT_ELEMENT_DESC InputLayouts::m_positionOnlyLayoutDesc[1] = 
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
UINT InputLayouts::m_positionTexLayoutNumElements = 2;
D3D11_INPUT_ELEMENT_DESC InputLayouts::m_positionTexLayoutDesc[2] = 
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
UINT InputLayouts::m_positionOnlyLayoutNumElements = 1;
D3D10_INPUT_ELEMENT_DESC InputLayouts::m_standardLayoutDesc10[4] = 
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0},
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D10_INPUT_PER_VERTEX_DATA, 0 },
};

HRESULT InputLayouts::Init(const UINT flags)
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"InputLayouts::Init");
		return E_FAIL;
	}

	HRESULT hr = S_OK;
	
	//STANDARD LAYOUT
	//Crear input (vertex) layout
	if(flags & INPUT_LAYOUT_STANDARD)
	{
		CompiledShader shader(m_d3dManager);
		D3DX11_PASS_DESC passDesc;

		if(FAILED(hr = shader.LoadPrecompiledShader(BASE_SHADER_FILE))) return hr;

		ID3DX11EffectTechnique *tmp = shader.GetEffect()->GetTechniqueByName("None");
		if(!tmp->IsValid()) { 
			DXGI_D3D_ErrorWarning(hr, L"InputLayouts::Init --> GetTechniqueByName");
			return E_FAIL; 
		}

		if(FAILED(hr = tmp->GetPassByIndex(0)->GetDesc(&passDesc))) {
			DXGI_D3D_ErrorWarning(hr, L"InputLayouts::Init --> GetDesc");
			return hr;
		}

		if(FAILED( hr = m_d3dManager.CreateInputLayout(m_standardLayoutDesc, m_standardLayoutNumElements, passDesc.pIAInputSignature,
		                                               passDesc.IAInputSignatureSize, &m_standardLayout) )) 
			return hr;
	}

	//POSITION ONLY LAYOUT (Para la sombra, renderizaciones auxiliares y el skybox)
	if(flags & INPUT_LAYOUT_POSITION_ONLY)
	{
		CompiledShader shader(m_d3dManager);
		D3DX11_PASS_DESC passDesc;

		if(FAILED(hr = shader.LoadPrecompiledShader(DIR_SHADOW_EFFECT_FILE))) return hr;

		ID3DX11EffectTechnique *tmp = shader.GetEffect()->GetTechniqueByName("BuildShadowMapTech");
		if(!tmp->IsValid()) { 
			DXGI_D3D_ErrorWarning(hr, L"InputLayouts::Init --> GetTechniqueByName");
			return E_FAIL; 
		}

		if(FAILED(hr = tmp->GetPassByIndex(0)->GetDesc(&passDesc))) {
			DXGI_D3D_ErrorWarning(hr, L"InputLayouts::Init --> GetDesc");
			return hr;
		}

		if(FAILED( hr = m_d3dManager.CreateInputLayout(m_positionOnlyLayoutDesc, m_positionOnlyLayoutNumElements, passDesc.pIAInputSignature,
		                                               passDesc.IAInputSignatureSize, &m_positionOnlyLayout) )) 
			return hr;
	}

	//POSITION AND TEX LAYOUT (Para el skybox)
	if(flags & INPUT_LAYOUT_POSITION_TEX)
	{
		CompiledShader shader(m_d3dManager);
		D3DX11_PASS_DESC passDesc;

		if(FAILED(hr = shader.LoadPrecompiledShader(SKY_BOX_EFFECT_FILE))) return hr;

		ID3DX11EffectTechnique *tmp = shader.GetEffect()->GetTechniqueByName("SkyBoxTechnique");
		if(!tmp->IsValid()) { 
			DXGI_D3D_ErrorWarning(hr, L"InputLayouts::Init --> GetTechniqueByName");
			return E_FAIL; 
		}

		if(FAILED(hr = tmp->GetPassByIndex(0)->GetDesc(&passDesc))) {
			DXGI_D3D_ErrorWarning(hr, L"InputLayouts::Init --> GetDesc");
			return hr;
		}

		if(FAILED( hr = m_d3dManager.CreateInputLayout(m_positionTexLayoutDesc, m_positionTexLayoutNumElements, passDesc.pIAInputSignature,
		                                               passDesc.IAInputSignatureSize, &m_positionTexLayout) )) 
			return hr;
	}

	m_ready = true;

	return hr;
}

ID3D11InputLayout *InputLayouts::GetStandardInputLayout() const
{
	return m_standardLayout;
}

ID3D11InputLayout *InputLayouts::GetPositionOnlyInputLayout() const
{
	return m_positionOnlyLayout;
}

ID3D11InputLayout *InputLayouts::GetPositionTexInputLayout() const
{
	return m_positionTexLayout;
}

const D3D11_INPUT_ELEMENT_DESC *InputLayouts::GetStandardLayoutDesc() const
{
	return m_standardLayoutDesc;
}
UINT InputLayouts::GetStandardLayoutNumElements() const
{
	return m_standardLayoutNumElements;
}

const D3D10_INPUT_ELEMENT_DESC *InputLayouts::GetMeshLayoutDesc() const
{
	return m_standardLayoutDesc10;
}

UINT InputLayouts::GetMeshLayoutNumElements() const
{
	return m_standardLayoutNumElements;
}

}