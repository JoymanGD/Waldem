#include "PBR.hlsl"

#define AMBIENT 0.07f

float3 GetDiffuseColor(float3 lightDir, float3 normal, float4 albedo, float4 orm, float4 reflection, float3 viewDirection)
{
    float3 diffuse = CookTorrenceBRDF(normal, viewDirection, lightDir, albedo.rgb, reflection.rgb, orm.g, orm.b);

    return diffuse;
}

float3 GetNormal(float3 normal, float3 tangent, float3 bitangent, float4 normalMap)
{
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return mul(normalMap.xyz * 2.0 - 1.0, TBN);
}