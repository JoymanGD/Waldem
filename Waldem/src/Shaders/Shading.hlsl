#include "Lighting.hlsl"
#include "Shadows.hlsl"
#include "PBR.hlsl"

float3 GetViewDirection(float4x4 cameraWorldTransform, float3 worldPos)
{
    float3 cameraPos = transpose(cameraWorldTransform)[3].xyz;
    float3 viewDir = normalize(cameraPos - worldPos);
    return viewDir;
}

float3 GetResultColor(Light light, Texture2D<float> Shadowmap, SamplerComparisonState cmpSampler, float4 worldPosition, float3 normal, float4 albedo, float4 roughnessMetallic, float4x4 cameraWorldTransform)
{
    float3 lightDir = -GetLightDirection(light);
    float3 viewDirection = GetViewDirection(cameraWorldTransform, worldPosition.xyz);
    float3 radiance = light.Color * light.Intensity * M_1_PI_F * saturate(dot(lightDir, normal));
    float3 ambient = AMBIENT * albedo.rgb;
    float3 diffuse = CookTorrenceBRDF(normal, viewDirection, lightDir, albedo.rgb, roughnessMetallic.g, roughnessMetallic.b);
    float shadowFactor = CalculateShadowFactor(Shadowmap, cmpSampler, worldPosition, normal, light.View, light.Projection);

    return ambient + diffuse * radiance * saturate(shadowFactor);
}