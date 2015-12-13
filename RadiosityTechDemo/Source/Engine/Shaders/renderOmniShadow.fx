//------------------------------------------------------------------------------------------
// File: renderOmniShadow.fx
//
// Se definen aquí los shaders necesarios para renderizar el shadow map para el algoritmo 
// de shadow mapping en luces omnidireccionales.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

cbuffer cbGeneralConstants
{
	matrix gWorld : WORLD;
	matrix gProj : PROJECTION;
	float gLightRange;
};

cbuffer cbPerFrame
{
	float3 gLightPosW;
	matrix gView[6];     //view matrices para la renderización del cube map
};


//--------------------------------------------------------------------------------------
// Estructuras de input / output 
//--------------------------------------------------------------------------------------

struct VS_IN
{
	float3 posL         : POSITION;
};

struct GS_IN
{
	float4 posW         : SV_POSITION;   //world position
};

struct PS_IN
{
	float4 posH         : SV_POSITION;   //clip space
	float3 posW         : POSITION;
	uint RenderTargetId : SV_RenderTargetArrayIndex;
};


//--------------------------------------------------------------------------------------
// Shaders
//--------------------------------------------------------------------------------------

GS_IN VS( VS_IN input )
{
	GS_IN output = (GS_IN) 0.0f;

	output.posW = mul(float4(input.posL, 1.0f), gWorld);

	return output;
}

[maxvertexcount(18)]
void GS( triangle GS_IN input[3], inout TriangleStream<PS_IN> stream )
{
   for(int f = 0; f < 6; ++f)
   {
		PS_IN output;
		output.RenderTargetId = f;    //definir render target
		for(int v = 0; v < 3; ++v)
		{
			output.posH = mul(mul(input[v].posW, gView[f]), gProj);    //clip space
			output.posW = input[v].posW.xyz;                           //world space
			stream.Append(output);
		}
		stream.RestartStrip();
	}
}

float PS( PS_IN input ) : SV_Target
{
	return length(gLightPosW - input.posW) / gLightRange;		//distancia a la luz normalizada
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 BuildShadowMapTech
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( CompileShader( gs_5_0, GS() ) );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
};

