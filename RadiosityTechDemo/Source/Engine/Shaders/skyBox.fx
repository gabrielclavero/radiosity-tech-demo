//------------------------------------------------------------------------------------------
// File: skyBox.fx
//
// Shaders para crear un sky box a partir de un sky texture.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Globales
//--------------------------------------------------------------------------------------

Texture2D SkyTexture;

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
// Estructuras de input / output 
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
    float4 posL     : POSITION;
    float2 texC     : TEXCOORD;
};

struct PS_INPUT
{
    float4 posL     : SV_Position;
    float2 texC     : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;

    output.posL = input.posL;
    output.texC = input.texC;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT input) : SV_Target
{
    return SkyTexture.Sample(LinearSampler, input.texC);
}


//--------------------------------------------------------------------------------------
// Technique
//--------------------------------------------------------------------------------------

technique10 SkyBoxTechnique
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}