//------------------------------------------------------------------------------------------
// File: Scene.h
//
// Carga una escena desde un archivo .txt. Crea la Mesh asociada y 
// configura las propiedades de la cámara y la luz de la escena según lo especificado
// en el archivo de entrada.
// También define dos funciones de renderización para dibujar toda la escena.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef SCENE_H
#define SCENE_H

#include <sstream>
#include <limits>

#include "Utility.h"
#include "D3DDevicesManager.h"

#include "Light.h"
#include "Camera.h"

#include "Mesh.h"
#include "CommonMaterialShader.h"

using std::vector;
using std::wstring;
using std::numeric_limits;
using std::streamsize;

namespace DTFramework
{

struct MeshProperties
{
	D3DXVECTOR3 pos;            //posición
	D3DXVECTOR3 rot;            //rotación
	wstring file;
	UINT num_vertices;          //cantidad total de vertices en el archivo .obj
	UINT num_normals;           //cantidad total de normales
	UINT num_texCoords;         //cantidad total de coordenadas de textura

	MeshProperties()
	: pos(D3DXVECTOR3(0, 0, 0)), rot(D3DXVECTOR3(0,0,0)), num_vertices(0), num_normals(0), num_texCoords(0)
	{

	}
};

class Scene
{
public:
	Scene(const D3DDevicesManager &);
	~Scene();

	//sólo debe llamarse a lo sumo una vez por objeto
	HRESULT Init(const wstring &sceneFile, Camera * const camera, Light * const light);

	HRESULT Render(const D3DXVECTOR3 * const cameraPos, const LightProperties * const light, const UINT activeLights, ID3D11ShaderResourceView *shadowMap,
	               const D3DXMATRIX * const lightVPM, ID3D11ShaderResourceView *GIData, const D3DXMATRIX * const viewProjection);

	HRESULT DrawSceneMesh() const;

	const Mesh *GetSceneMesh() const;

	UINT GetShadowMapsSize() const;
	float GetScale() const;

	bool ShowSky() const;

private:
	HRESULT LoadSceneFromFile(const wstring &sceneFile, Camera * const camera, Light * const light);

private:
	static const float Z_FAR;
	static const float Z_NEAR;
	static const UINT SHADOW_MAP_SIZE;

	static const float LIGHT_VOLUME_WIDTH;
	static const float LIGHT_VOLUME_HEIGHT;
	static const float LIGHT_Z_NEAR;
	static const float LIGHT_Z_FAR;

	static const float TRANSPARENCY_BOUNDARY;		//materiales con un alfa menor a este valor no proyectan sombra

	enum SceneFileError { SCENE_FILE_ERROR };

	const D3DDevicesManager &m_d3dManager;

	//material shaders
	CommonMaterialShader m_commonShader;

	//objeto mesh
	Mesh *m_sceneMesh;
	MeshProperties m_sceneMeshProperties;

	//propiedades de la escena
	float m_zFar;
	float m_zNear;
	UINT m_shadowMapsSize;
	float m_scale;
	bool m_showSky;

	bool m_ready;
};

inline UINT Scene::GetShadowMapsSize() const
{
	return m_shadowMapsSize;
}

inline float Scene::GetScale() const
{
	return m_scale;
}

inline bool Scene::ShowSky() const
{
	return m_showSky;
}

inline const Mesh *Scene::GetSceneMesh() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Scene::GetSceneMesh");
		return NULL;
	}

	return m_sceneMesh;
}

}

#endif