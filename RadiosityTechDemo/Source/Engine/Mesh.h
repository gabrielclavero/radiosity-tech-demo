//------------------------------------------------------------------------------------------
// File: Mesh.h
//
// La clase Mesh crea una ID3DX10Mesh desde un archivo .obj y sus Materials asociados
// desde un archivo .mtl. Luego utiliza la ID3DX10Mesh optimizada para crear los vertex e 
// index buffers que puedan ser usados con DirectX11.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef MESH_H
#define MESH_H

#include <sstream>
#include <ctime>
#include <cctype>
#include <limits>

#include "Utility.h"
#include "Material.h"
#include "InputLayouts.h"
#include "D3DDevicesManager.h"
#include "D3D11Resources.h"

#define ERROR_RESOURCE_VALUE 1

using std::vector;
using std::bad_alloc;
using std::stringstream;
using std::endl;
using std::wstring;
using std::numeric_limits;
using std::streamsize;

namespace DTFramework
{

template<typename TYPE> BOOL IsErrorResource( TYPE data )
{
	if( ( TYPE )ERROR_RESOURCE_VALUE == data )
		return TRUE;
	return FALSE;
}

class Mesh 
{
public:
	Mesh(const D3DDevicesManager &d3d);
	~Mesh();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init( const wstring &meshFile, const UINT numVertices, const UINT numNormals, const UINT numCoords );

	HRESULT Render(const UINT subset) const;

	const wstring &GetFileName() const;

	UINT GetNumMaterials() const;
	UINT GetAttributeTableEntries() const;

	//devuelve el material usado por el i-ésimo subset de la mesh
	const Material * const GetSubsetMaterial(const UINT i) const;

	UINT GetTotalFaces() const;
	UINT GetTotalVertices() const;

	ID3DX10Mesh *GetID3DX10Mesh() const;
	ID3D11Buffer *GetVertexBuffer() const;

private:
	void SetTechniquesForMaterials();
	HRESULT LoadGeometryFromOBJ( const wstring &strFileName,  const UINT numVertices, const UINT numNormals, const UINT numCoords );
	HRESULT LoadMaterialsFromMTL( const wstring &strFileName );

	DWORD AddVertex(const UINT a, const Vertex &v);

	HRESULT CalculateTangents();

private:
	const D3DDevicesManager &m_d3dManager;

	enum OBJMTLError { OBJFILE_ERROR, MTLFILE_ERROR };		//excepciones para errores en archivos .obj y .mtl

	static const UINT HASH_TABLE_SIZE = 8192;
	vector<UINT> m_hashTable[HASH_TABLE_SIZE];

	wstring m_meshFile;

	//lista de materiales en la mesh
	vector<Material> m_materials;

	//malla completa
	vector<VERTEX> m_vertices;
	vector<DWORD> m_indices;
	vector<DWORD> m_attributes;

	UINT m_numAttribTableEntries;
	D3DX10_ATTRIBUTE_RANGE *m_pAttribTable;

	ID3DX10Mesh *m_mesh;

	VertexBuffer *m_vertexBuffer;
	IndexBuffer *m_indexBuffer;

	UINT m_totalVertices;
	UINT m_totalFaces;

	bool m_ready;
};

inline const wstring &Mesh::GetFileName() const
{
	return m_meshFile;
}
inline UINT Mesh::GetNumMaterials() const
{
	return static_cast<UINT> ( m_materials.size() );
}
inline UINT Mesh::GetAttributeTableEntries() const
{
	return m_numAttribTableEntries;
}
inline UINT Mesh::GetTotalFaces() const
{
	return m_totalFaces;
}
inline UINT Mesh::GetTotalVertices() const
{
	return m_totalVertices;
}
inline const Material * const Mesh::GetSubsetMaterial(const UINT i) const
{
	_ASSERT(i < m_numAttribTableEntries);

	if(i >= m_numAttribTableEntries) {
		MiscErrorWarning(INVALID_PARAMETER, L"Mesh::GetSubsetMaterial");
		return NULL;
	}

	return &(m_materials[ m_pAttribTable[i].AttribId ]);
}
inline ID3DX10Mesh *Mesh::GetID3DX10Mesh() const
{
	return m_mesh;
}
inline ID3D11Buffer *Mesh::GetVertexBuffer() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Mesh::GetVertexBuffer");
		return NULL;
	}

	return m_vertexBuffer->GetBuffer();
}


}

#endif