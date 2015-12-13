//------------------------------------------------------------------------------------------
// File: commonMaterialShader.fx
//
// Vertex y Pixel shaders para materiales comunes.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


#include "lights.fx"
#include "base.fx"

//--------------------------------------------------------------------------------------
// Pixel shader input structure
//--------------------------------------------------------------------------------------

struct PS_INPUT
{
	float4 posH         : SV_POSITION;
	float3 posW         : POSITION;

	float3 tangentW     : TANGENT;
	float3 normalW      : NORMAL;

	float2 texC         : TEXCOORD0;

	float4 projTexC     : TEXCOORD1;	//coordenadas para las sombras si la luz en escena es direccional

	float3 giIrradiance : GI;
};


//--------------------------------------------------------------------------------------
// Vertex shader
//--------------------------------------------------------------------------------------

PS_INPUT VS( VS_INPUT input, in uint VertexID : SV_VertexID )
{
    PS_INPUT output;

	//posición, tangente y normal necesitan estar en world space para iluminación
	//pero world es identidad de manera que solo copiamos
	output.posW = input.posL;                 //mul(float4(input.posL, 1.0f), gWorld).xyz;
	output.tangentW = input.tangentL;         //mul(float4(input.tangentL, 0.0f), gWorld).xyz;
	output.normalW = input.normalL;	          //mul(float4(input.normalL, 0.0f), gWorld).xyz;

	//posicion a clip space para display
	output.posH = mul(float4(input.posL, 1.0f), gWVP);

	//pasamos la coordenada de la textura
	output.texC = input.texC;

	//inicializar projTexC
	output.projTexC = float4(0,0,0,0);

	//sombras
	if(gLight.type != POINT_LIGHT)
		output.projTexC = mul(float4(input.posL, 1.0f), gLightWVP);	

	//GI
	output.giIrradiance = gGILightInfoPerVertex.Load(VertexID).rgb;

    return output;
}


//--------------------------------------------------------------------------------------
// Pixel shader
//--------------------------------------------------------------------------------------

float4 PS( PS_INPUT input, uniform bool useDiffuseTexture, uniform bool useNormalTexture, uniform bool useSpecularLight, uniform bool useGILight ) : SV_Target
{
	SurfaceInfo v;

	//vector normal
	float3 normalT = float3(1,1,1);
	if(useNormalTexture) 
	{
		//construir matriz ortonormal
		float3 N = normalize(input.normalW);
		float3 T = normalize(input.tangentW - dot(input.tangentW, N) * N);
		float3 B = cross(N,T);

		float3x3 TBN = float3x3(T,B,N);

		normalT = gNormalTexture.Sample(LinearSampler, input.texC).xyz;

		//descomprimir de [0,1] a [-1,1]
		normalT = 2.0f * normalT - 1.0f;

		//transformar desde tangent (texture) space a world space
		float3 bumpedNormalW = normalize(mul(normalT, TBN));

		//la normal que usaremos para computar la luz de este pixel es extraída del normal map
		v.normal = bumpedNormalW;
	} 
	else 
	{
		v.normal = normalize(input.normalW);
	}

	//resto de las propiedades de la superficie
	v.pos = input.posW;
	v.ambient = float4(gMaterialAmbient, 0);
	v.diffuse = float4(gMaterialDiffuse, 0);
	v.specular = float4(gMaterialSpecular, 0);
	v.shininess = gMaterialShininess;

	//iluminación directa
	float3 litColor = 0;
	if(gActiveLights > 0)			//no hay divergencia. La luz está encendida o apagada para todos los pixeles.
	{    
		if(gLight.type == DIRECTIONAL_LIGHT)
			litColor = DirectionalLight(v, gLight, gCameraPosition, input.projTexC, useSpecularLight);
		else if(gLight.type == POINT_LIGHT)
			litColor = PointLight(v, gLight, gCameraPosition, useSpecularLight);
	}
	
	//textura difusa
	float3 textureColor = float3(1,1,1);
	if(useDiffuseTexture) 
	{
		textureColor = gDiffuseTexture.Sample(LinearSampler, input.texC).xyz;
	} 

	//si no hay iluminación indirecta este es el color final del pixel
	if(!useGILight)
		return float4(litColor*textureColor, gMaterialAlpha);
	

	//iluminación indirecta activada
	float3 GILight = input.giIrradiance;

	float3 finalColor = (GILight * v.diffuse + litColor)*textureColor;

	return float4(max(min(finalColor.r, 1.0f), 0), max(min(finalColor.g, 1.0f), 0), max(min(finalColor.b, 1.0f), 0), gMaterialAlpha);
}



//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 None
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, false, false, false) ) );
    }
}
technique10 Specular
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, false, true, false) ) );
    }
}
technique10 Diffuse
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, false, false, false) ) );
    }
}
technique10 Normal
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, true, false, false) ) );
    }
}
technique10 NoneGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, false, false, true) ) );
    }
}
technique10 SpecularGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, false, true, true) ) );
    }
}
technique10 NormalGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, true, false, true) ) );
    }
}
technique10 NormalSpecular
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, true, true, false) ) );
    }
}
technique10 NormalSpecularGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(false, true, true, true) ) );
    }
}
technique10 DiffuseGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, false, false, true) ) );
    }
}
technique10 DiffuseSpecular
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, false, true, false) ) );
    }
}
technique10 DiffuseSpecularGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, false, true, true) ) );
    }
}
technique10 DiffuseNormal
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, true, false, false) ) );
    }
}
technique10 DiffuseNormalGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, true, false, true) ) );
    }
}
technique10 DiffuseNormalSpecular
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, true, true, false) ) );
    }
}
technique10 DiffuseNormalSpecularGI
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(true, true, true, true) ) );
    }
}