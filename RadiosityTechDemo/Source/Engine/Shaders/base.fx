//------------------------------------------------------------------------------------------
// File: base.fx
//
// Constantes generales, constant buffers, samplers y estructura de vértice comunes a
// todos los shaders de materiales.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------
// Constantes
//--------------------------------------------------------------------------------------

static const int DIRECTIONAL_LIGHT = 0;
static const int POINT_LIGHT = 1;

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

cbuffer cbPerMaterial
{
	float3 gMaterialAmbient;
	float3 gMaterialDiffuse;
	float3 gMaterialSpecular;
	float  gMaterialAlpha;
	float  gMaterialShininess;
};

cbuffer cpPerObject
{
	//matrix gWorld;                    // World matrix siempre la identidad
	matrix gWVP;                        // World * View * Projection matrix
};

cbuffer cbPerFrame
{
	float3 gCameraPosition;             //posicion de la camara en world coords
	uint gActiveLights;                 //numero de luces activas en la escena. 0 o 1 puesto que sólo hay una por escena
};

//lights
cbuffer cbLights
{
	Light gLight;
	matrix gLightWVP;                   //world view projection matrix de la luz si ésta es direccional
};


//--------------------------------------------------------------------------------------
// Texturas y Buffers
//--------------------------------------------------------------------------------------

Texture2D gDiffuseTexture;              //textura color difuso para la mesh
Texture2D gNormalTexture;               //textura normal para la mesh

Buffer<float4> gGILightInfoPerVertex;   //datos de iluminación indirecta de cada vértice


//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------

SamplerState AnisotropicSampler
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
};


SamplerState LinearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};


//--------------------------------------------------------------------------------------
// Vertex shader input structure
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float3 posL     : POSITION;
	float3 normalL  : NORMAL;
	float3 tangentL : TANGENT;
	float2 texC     : TEXCOORD;
};
