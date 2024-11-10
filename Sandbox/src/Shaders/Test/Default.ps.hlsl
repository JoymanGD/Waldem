#include "Core.hlsl"

struct Light
{
    float3 Position;
    uint Type;
    float3 Direction;
    float Intensity;
    float3 Color;
    float Range;
    matrix ViewProjection;
};

#define SHADOW_BIAS 0.001f
#define AMBIENT 0.8f

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
Texture2D Shadowmap : register(t1);
Texture2D DiffuseTextures[MAX_TEXTURES] : register(t2);

float CalculateShadowFactor(float4 worldPos, float4x4 lightViewProjection)
{
    float4 lightClipPos = mul(lightViewProjection, worldPos);

    float2 shadowUVRaw = lightClipPos.xy / lightClipPos.w;
    
    float2 shadowUV = (shadowUVRaw + 1.0f) / 2.0f;

    float shadowDepth = Shadowmap.Sample(myStaticSampler, shadowUV).r;

    float currentDepth = lightClipPos.z / lightClipPos.w;

    return currentDepth > shadowDepth ? 0.0f : 1.0f;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);
    
    if(color.a < 0.1f)
        discard;

    Light light = Lights[0];

    float shadowFactor = CalculateShadowFactor(input.WorldPosition, light.ViewProjection);

    float3 resultColor = color.rgb * AMBIENT + light.Color * light.Intensity * saturate(dot(input.Normal, -light.Direction)) * shadowFactor;

    return float4(resultColor, 1.0f);
}
