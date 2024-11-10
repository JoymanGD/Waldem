#include "Core.hlsl"

#define SHADOW_BIAS 0.001f
#define AMBIENT 0.2f

struct Light
{
    float3 Color;
    float Intensity;
    uint Type;
    float Range;
    float2 Padding1;
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

cbuffer MyConstantBuffer : register(b0)
{
    matrix world;
    matrix vp;
};

SamplerState myStaticSampler : register(s0);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D DiffuseTextures[MAX_TEXTURES] : register(t2);

float3 GetLightDirection(Light light)
{
    float3 forward = transpose(light.World)[2];
    return normalize(forward);
}

float CalculateShadowFactor(float4 worldPos, float4x4 view, float4x4 projection)
{
    float4 lightClipPos = mul(view, worldPos);
    lightClipPos = mul(projection, lightClipPos);
    
    float2 shadowUVRaw = lightClipPos.xy;
    
    float2 shadowUV = shadowUVRaw * 0.5f + 0.5f;
    shadowUV.y = -shadowUV.y;

    float shadowDepth = Shadowmap.Sample(myStaticSampler, shadowUV).r;

    float currentDepth = lightClipPos.z - SHADOW_BIAS;

    return currentDepth > shadowDepth ? 0.0f : 1.0f;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);
    
    if(color.a < 0.1f)
        discard;

    Light light = Lights[0];

    float shadowFactor = CalculateShadowFactor(input.WorldPosition, light.View, light.Projection);

    float3 lightDirection = GetLightDirection(light);

    float3 resultColor = color.rgb * AMBIENT + color.rgb * light.Color * light.Intensity * saturate(dot(input.Normal, -lightDirection)) * shadowFactor;

    return float4(resultColor, 1.0f);
}
