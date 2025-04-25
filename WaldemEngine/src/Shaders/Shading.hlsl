#include "PBR.hlsl"

#define AMBIENT 0.07f

float3 GetViewDirection(float4x4 cameraWorldTransform, float3 worldPos)
{
    float3 cameraPos = transpose(cameraWorldTransform)[3].xyz;
    float3 viewDir = normalize(cameraPos - worldPos);
    return viewDir;
}

float3 GetDiffuseColor(float3 lightDir, float4 worldPosition, float3 normal, float4 albedo, float4 orm, float4x4 cameraWorldTransform)
{
    float3 viewDirection = GetViewDirection(cameraWorldTransform, worldPosition.xyz);
    float3 diffuse = CookTorrenceBRDF(normal, viewDirection, lightDir, albedo.rgb, orm.g, orm.b);

    return diffuse;
}