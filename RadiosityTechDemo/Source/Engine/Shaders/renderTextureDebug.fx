//------------------------------------------------------------------------------------------
// File: renderTextureDebug.fx
//
// Shaders para leer y mostrar la información de texturas 2d.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Globales
//--------------------------------------------------------------------------------------

Texture2D gMap;
TextureCube gEnvMap;

bool gSingleDepth;

//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------

SamplerState LinearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

//--------------------------------------------------------------------------------------
// Estructuras de input/output para los shaders
//--------------------------------------------------------------------------------------

struct VS_IN
{
	float3 posL     : POSITION;
	float3 normal   : NORMAL;
	float3 tangent  : TANGENT;
	float2 texC     : TEXCOORD;
};

struct VS_OUT
{
	float4 posH     : SV_POSITION;
	float3 normal   : NORMAL;
	float2 texC     : TEXCOORD;
};
 

//--------------------------------------------------------------------------------------
// Shaders
//--------------------------------------------------------------------------------------

VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;

	vOut.posH = float4(vIn.posL, 1.0f);
	
	vOut.texC = vIn.texC;
	vOut.normal = vIn.normal;
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	float r = 0;

	if(gSingleDepth) {
		//single depth debug
		r = gMap.Sample(LinearSampler, pIn.texC).r;
	} else {
		//omni debug
		r = gEnvMap.Sample(LinearSampler, float3(pIn.texC, pIn.texC.x)).r;
	}
	
	return float4(r,r,r, 1);
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 DrawMapTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS() ) );
    }
}
