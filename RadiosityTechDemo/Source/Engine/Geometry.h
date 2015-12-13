//------------------------------------------------------------------------------------------
// File: Geometry.h
//
// Definiciones de vértices y entidades geométricas.
//
// Author: Gabriel Clavero
// 
//------------------------------------------------------------------------------------------

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <d3dx10math.h>

namespace DTFramework
{

typedef struct Vertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	D3DXVECTOR3 tangent;
	D3DXVECTOR2 texcoord;
} VERTEX, Vertex;

struct GIVertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	D3DXVECTOR3 tangent;
	D3DXVECTOR3 bitangent;
};

struct SimpleVertex
{
	D3DXVECTOR4	position;
	D3DXVECTOR2 texCoord;
};


}

#endif