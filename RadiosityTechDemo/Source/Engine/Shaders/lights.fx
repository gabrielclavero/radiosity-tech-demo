//------------------------------------------------------------------------------------------
// File: lights.fx
//
// Funciones BRDF Phong y Blinn-Phong.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "shadowFunctions.fx"

struct Light
{
	float3 pos;
	float3 dir;
	float4 ambient;
	float4 diffuse;
	float4 spec;
	float3 att;
	float range;
	int type;
	float shadowMapBias;
	int on;
	float pad;
};

struct SurfaceInfo
{
	float3 pos;
	float3 normal;
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float shininess;
};

//--------------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------------

float3 ComputeSpecularPhong(uniform float3 specularFactor, uniform float3 lightVec, 
                            uniform float3 viewer, uniform float3 normal, uniform float shininess)
{
	float specPower = max(shininess, 1.0f);
	float3 R = reflect(-lightVec, normal);
	float specFactor = pow(max(dot(R, viewer), 0.0f), specPower);

	return (specFactor * specularFactor);
}

float3 ComputeSpecularBlinnPhong(uniform float3 specularFactor, uniform float3 lightVec, 
                                 uniform float3 viewer, uniform float3 normal, uniform float shininess)
{
	float3 half_vector = normalize(lightVec + viewer);

	float HdotN = max( 0.0f, dot(half_vector, normal));
	
	float specPower	= max(shininess, 1.0f);
	float3 specularTerm = specularFactor * pow(HdotN, specPower);

	return specularTerm;
}


//--------------------------------------------------------------------------------------
// Funciones para cada tipo de luz
//--------------------------------------------------------------------------------------

float3 DirectionalLight(uniform SurfaceInfo v, uniform Light L, uniform float3 eyePos, uniform float4 projTexC, uniform bool bSpecular)
{
	//vectores dirección de la luz y al observador
	float3 lightVec = normalize(L.pos - L.dir);
	float3 viewer = normalize(eyePos - v.pos);

	//color ambiental
	float3 ambientTerm = (v.ambient * L.ambient).xyz;

	//color difuso
	float diffuseFactor = max(0.0f, dot(v.normal, lightVec));		//v.normal y lightVec son ambos versores

	float3 litColor = (L.diffuse * v.diffuse).rgb * diffuseFactor;	//término difuso
	
	//color especular
	if(bSpecular && diffuseFactor > 0) {
		float3 specularFactor = (v.specular * L.spec).rgb;
		litColor += ComputeSpecularBlinnPhong(specularFactor, lightVec, viewer, v.normal, v.shininess);
	}

	//shadow
	float shadowFactor = 1.0f;
	shadowFactor = CalcShadowFactor(projTexC, L.shadowMapBias);

	//color final
	return L.on * (litColor * shadowFactor + ambientTerm);
}

float3 PointLight(uniform SurfaceInfo v, uniform Light L, uniform float3 eyePos, uniform bool bSpecular)
{	
	float3 viewer = normalize(eyePos - v.pos);

	//color ambiental
	float3 ambientTerm = (v.ambient * L.ambient).xyz;	
	
	//vector desde el punto en consideracion hasta la luz (no consideramos la dirección de la luz porque las point no tienen dirección)
	float3 lightVec = L.pos - v.pos;
		
	//distancia desde el punto hasta la luz
	float d = length(lightVec);
	
	//si salimos del rango de la point light devolvemos sólo color ambiental
	if(d > L.range)
		return ambientTerm;
		
	//normalizar
	lightVec /= d; 

	//color difuso
	float diffuseFactor = max(0.0f, dot(v.normal, lightVec));		//v.normal y lightVec son ambos versores

	float3 litColor = (L.diffuse * v.diffuse).rgb * diffuseFactor;	//término difuso
	
	//color especular
	if(bSpecular && diffuseFactor > 0) {
		float3 specularFactor = (v.specular * L.spec).rgb;
		litColor += ComputeSpecularBlinnPhong(specularFactor, lightVec, viewer, v.normal, v.shininess);
	}

	//shadow
	float shadowFactor = 1.0f;
	shadowFactor = CalcOmniShadowFactor(lightVec, d, L.range);

	//color final
	litColor = litColor * shadowFactor + ambientTerm;
	
	//atenuar
	return L.on * ( litColor / dot(L.att, float3(1.0f, d/256.0f, d*d)) );
}

