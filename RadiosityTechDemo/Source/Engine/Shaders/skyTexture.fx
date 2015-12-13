//------------------------------------------------------------------------------------------
// File: skyTexture.fx
//
// Shaders para crear una sky texture.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Globales
//--------------------------------------------------------------------------------------

matrix gView;
matrix gProjection;
float3 gBias;
float3 gSunDir;


//--------------------------------------------------------------------------------------
// Estructuras Input Output
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float3 posL     : POSITION;
};

struct PS_INPUT
{
	float4 posH     : SV_Position;
	float3 posL     : POSITION;
};
	
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	//pasar posición del pixel
	output.posL = input.posL;

	//view space con centro en (0,0,0)
	float3 posV = mul(input.posL, (float3x3) gView);

	//clip space
	output.posH = mul(float4(posV, 1.0f), gProjection);

	return output;
}

//-------------------------------------------------------------------------------------------------
// Modelo estándar CIE
//
// Ver http://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf 
// Sección 2.3 Ecuación (1) 
//-------------------------------------------------------------------------------------------------

float3 CIEStandardSky(float3 viewer, float3 sunDirection)
{
	const float cosThetaS = dot(sunDirection, float3(0, 1, 0));
	const float cosGamma = dot(viewer, sunDirection);
	const float cosTheta = dot(viewer, float3(0, 1, 0));

	const float gamma = acos(cosGamma);       //ángulo entre viewer y sun direction
	const float theta = acos(cosTheta);       //ángulo entre viewer y la normal
	const float thetaS = acos(cosThetaS);     //ángulo entre sunDirection y la normal

	//luminancia
	const float Yc = ( (0.91f + 10 * exp(-3 * gamma) + 0.45 * cosGamma * cosGamma) * (1 - exp(-0.32f / cosTheta )) )
	                  / ( (0.91f + 10 * exp(-3 * thetaS) + 0.45 * cosThetaS * cosThetaS) * (1 - exp(-0.32f)) );

	const float3 skyColor = float3(0.25f, 0.65f, 1.0f);     //Yc no posee información de longitud de onda

	//combinamos la luminancia con el color del cielo 
	return max(skyColor * Yc, 0);
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT input) : SV_Target
{
	float3 skyLuminance = CIEStandardSky(normalize(input.posL), gSunDir);
	return float4(gBias * skyLuminance, 1.0f);
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 SkyTextureTechnique
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}