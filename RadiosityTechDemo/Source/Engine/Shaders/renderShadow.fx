//------------------------------------------------------------------------------------------
// File: renderShadow.fx
//
// Se definen aquí los shaders necesarios para renderizar el shadow map para el algoritmo 
// de shadow mapping en luces direccionales.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------



cbuffer cbPerFrame
{
	float4x4 gLightWVP;
};

struct VS_IN
{
	float3 posL     : POSITION;
};

struct VS_OUT
{
	float4 posH     : SV_POSITION;
};

VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;

	vOut.posH = mul(float4(vIn.posL, 1.0f), gLightWVP);

	return vOut;
}

void PS(VS_OUT pIn)
{
	
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 BuildShadowMapTech
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}