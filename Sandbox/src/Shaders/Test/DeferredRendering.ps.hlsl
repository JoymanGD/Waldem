#include "Core.hlsl"

#define SHADOW_BIAS 0.0001f
#define AMBIENT 0.2f

struct Light
{
    float3 Color;
    float Intensity;
    float2 Padding1;
    uint Type;
    float Range;
    matrix World;
    matrix View;
    matrix Projection;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

SamplerState myStaticSampler : register(s0);
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D WorldPosition : register(t2);
Texture2D Normal : register(t3);
Texture2D Albedo : register(t4);

float3 GetLightDirection(Light light)
{
    float3 forward = transpose(light.World)[2];
    return normalize(forward);
}

float CalculateShadowFactor(float3 worldPos, float4x4 view, float4x4 projection)
{
    float4 lightClipPos = mul(view, float4(worldPos, 1.0f));
    lightClipPos = mul(projection, lightClipPos);

    lightClipPos.xyz /= lightClipPos.w;
    
    float2 shadowUV = lightClipPos.xy * 0.5f + 0.5f;
    shadowUV.y = -shadowUV.y;

    float depth = lightClipPos.z - SHADOW_BIAS;

    float sum = 0;
    
    float2 shadowMapSize = float2(2048, 2048);
    float xOffset = 1.0/shadowMapSize.x;
    float yOffset = 1.0/shadowMapSize.y;
 
    //perform PCF filtering on a 4 x 4 texel neighborhood
    for (int y = -1; y <= 1; y += 1)
    {
        for (int x = -1; x <= 1; x += 1)
        {
            float2 Offsets = float2(x * xOffset, y * yOffset);
            sum += Shadowmap.SampleCmpLevelZero(cmpSampler, shadowUV + Offsets, depth);
        }
    }

    return sum / 18.0f;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 worldPosition = WorldPosition.Sample(myStaticSampler, input.UV).xyz;
    float3 normal = Normal.Sample(myStaticSampler, input.UV).xyz;
    float4 albedo = Albedo.Sample(myStaticSampler, input.UV);

    Light light = Lights[0];

    float shadowFactor = CalculateShadowFactor(worldPosition, light.View, light.Projection);
    
    float3 lightDirection = GetLightDirection(light);

    float3 resultColor = albedo.rgb * AMBIENT + albedo.rgb * light.Color * light.Intensity * saturate(dot(normal, -lightDirection)) * saturate(shadowFactor);
    // float3 resultColor = float3(1.0f,0,0);

    return float4(resultColor, 1.0f);
}
