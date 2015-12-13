//------------------------------------------------------------------------------------------
// File: depthOnly.fx
//
// Vertex y pixel shader usados para renderizar sólo la profundidad de la geometría.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

cbuffer cbPerFrame
{
	float4x4 gWVP;
};

struct VS_IN
{
	float3 posL    : POSITION;
};

struct VS_OUT
{
	float4 posH    : SV_POSITION;
};

VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;

	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP);

	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	return float4(0,0,0,1);
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 DepthOnlyRenderTechnique
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}