//------------------------------------------------------------------------------------------
// File: InputLayouts.h
//
// Definimos los input layouts que se necesiten para los distintos shaders.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef INPUT_LAYOUTS_H
#define INPUT_LAYOUTS_H

#include "Utility.h"
#include "D3DDevicesManager.h"
#include "CompiledShader.h"

#include "d3dx11effect.h"

namespace DTFramework
{

namespace
{
	const UINT INPUT_LAYOUT_STANDARD = 1;
	const UINT INPUT_LAYOUT_POSITION_ONLY = 2;
	const UINT INPUT_LAYOUT_POSITION_TEX = 4;
}

class InputLayouts
{
public:
	InputLayouts(const D3DDevicesManager &d3d);
	~InputLayouts();

	//inicializa los input layouts especificados en flags. Sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init(const UINT flags);

	ID3D11InputLayout *GetStandardInputLayout() const;
	ID3D11InputLayout *GetPositionOnlyInputLayout() const;
	ID3D11InputLayout *GetPositionTexInputLayout() const;

	const D3D11_INPUT_ELEMENT_DESC *GetStandardLayoutDesc() const;
	UINT GetStandardLayoutNumElements() const;

	//para creación del ID3DX10Mesh
	const D3D10_INPUT_ELEMENT_DESC *GetMeshLayoutDesc() const;
	UINT GetMeshLayoutNumElements() const;

private:
	const D3DDevicesManager &m_d3dManager;

	ID3D11InputLayout *m_standardLayout;        //input layout con posicion, normal, tangente y tex coord
	ID3D11InputLayout *m_positionOnlyLayout;    //input layout con posicion
	ID3D11InputLayout *m_positionTexLayout;     //input layout con posicion y tex coords

	bool m_ready;

	//STANDARD LAYOUT
	static D3D11_INPUT_ELEMENT_DESC m_standardLayoutDesc[4];
	static UINT m_standardLayoutNumElements;

	//POSITION LAYOUT
	static D3D11_INPUT_ELEMENT_DESC m_positionOnlyLayoutDesc[1];
	static UINT m_positionOnlyLayoutNumElements;

	//POSITION TEX LAYOUT
	static D3D11_INPUT_ELEMENT_DESC m_positionTexLayoutDesc[2];
	static UINT m_positionTexLayoutNumElements;

	//STANDARD LAYOUT D3D10 (Para creación del ID3DX10Mesh)
	static D3D10_INPUT_ELEMENT_DESC m_standardLayoutDesc10[4];
};

inline InputLayouts::InputLayouts(const D3DDevicesManager &d3d)
: m_d3dManager(d3d), m_standardLayout(0), m_positionOnlyLayout(0), m_positionTexLayout(0), m_ready(false)
{

}

inline InputLayouts::~InputLayouts()
{
	SAFE_RELEASE(m_standardLayout);
	SAFE_RELEASE(m_positionOnlyLayout);
	SAFE_RELEASE(m_positionTexLayout);
}

}

#endif