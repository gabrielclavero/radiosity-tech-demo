//------------------------------------------------------------------------------------------
// File: shadowFunctions.fx
//
// Funciones para calcular el factor de sombra de luces direccionales u omnidireccionales.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Globales
//--------------------------------------------------------------------------------------

Texture2D gShadowMap;
TextureCube gOmniShadowMap;

//--------------------------------------------------------------------------------------
// Constantes
//--------------------------------------------------------------------------------------
static const float SMAP_SIZE = 800.0f;

//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------

SamplerComparisonState ComparisonSampler
{
	Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
	AddressU = Mirror;
	AddressV = Mirror;
	ComparisonFunc = LESS_EQUAL;
};

//--------------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------------

float2 texOffsetx2(int u, int v)
{
	return float2( u * 1.0f/SMAP_SIZE, v * 1.0f/SMAP_SIZE );
}

float3 texOffsetx3(int u, int v, int w)
{
	return float3( u * 1.0f/SMAP_SIZE, v * 1.0f/SMAP_SIZE, w * 1.0f/SMAP_SIZE );
}

//------------------------------------------------------------------------------------------
// shadow factor para luz direccional. 
//------------------------------------------------------------------------------------------
float CalcShadowFactor(float4 projTexC, float shadowMapBias)
{	
	//completar proyección dividiendo por w
	projTexC.xyz /= projTexC.w;
	
	//puntos fuera del volumen de la luz estan en sombra
	if( projTexC.x < -1.0f || projTexC.x > 1.0f || projTexC.y < -1.0f || projTexC.y > 1.0f || projTexC.z < 0.0f || projTexC.z > 1.0f ) 
	    return 0.0f;
	
	    
	//transformar de NDC space a texture space
	projTexC.x = +0.5f*projTexC.x + 0.5f;
	projTexC.y = -0.5f*projTexC.y + 0.5f;

	//shadow map bias para remover artifacts
	projTexC.z -= shadowMapBias;

	//PCF sampling
	float sum = 0;
	float x, y;

	//filtro PCF considerando 4x4 texels.
	for(y = -1.5; y <= 1.5; y += 1.0) {
		for(x = -1.5; x <=1.5; x += 1.0) {
			//recordar que en la coordenada z esta la profundidad del pixel por eso
			//comparamos el depth value del shadow map contra el valor projTexC.z de clip space
			sum += gShadowMap.SampleCmpLevelZero(ComparisonSampler, projTexC.xy + texOffsetx2(x,y), projTexC.z);			                                                                                                        
		}
	}

	return sum / 16.0;
}


//------------------------------------------------------------------------------------------
// shadow factor para luz omnidireccional.
//
// pixelToLight: vector del pixel a la luz (normalizado)
// pixelToLightLength: longitud del vector del pixel a la luz (sin normalizar)
//------------------------------------------------------------------------------------------
float CalcOmniShadowFactor(float3 pixelToLightNormalized, float pixelToLightLength, float lightRange)
{	
	float pixelDepth = pixelToLightLength / lightRange;		//la profundidad del pixel es la distancia a la luz, y debemos dividirlo por el rango de la luz

	float sum = 0;
	float x, y, z;
	float3 lookUpVector = -pixelToLightNormalized;

	if(abs(lookUpVector.z) > abs(lookUpVector.x)) {			//solo queremos samplear texels del plano que hayamos intersecado con nuestro look up vector
		for(y = -1.5; y <= 1.5; y += 1.0) {
			for(x = -1.5; x <= 1.5; x += 1.0) {
				sum += gOmniShadowMap.SampleCmpLevelZero(ComparisonSampler, lookUpVector + texOffsetx3(x,y,0), pixelDepth);
			}
		}
	} else {
		for(y = -1.5; y <= 1.5; y += 1.0) {
			for(z = -1.5; z <= 1.5; z += 1.0) {
				sum += gOmniShadowMap.SampleCmpLevelZero(ComparisonSampler, lookUpVector + texOffsetx3(0,y,z), pixelDepth);
			}
		}
	}
	

	return sum / 16.0;
}