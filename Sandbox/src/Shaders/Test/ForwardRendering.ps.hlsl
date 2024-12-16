#include "Lighting.hlsl"
#include "Shadows.hlsl"

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
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D DiffuseTextures[MAX_TEXTURES] : register(t3);

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);
    
    if(color.a < 0.1f)
        discard;

    Light light = Lights[0];

    float shadowFactor = CalculateShadowFactor(Shadowmap, cmpSampler, input.WorldPosition, light.View, light.Projection);
    
    float3 lightDirection = GetLightDirection(light);

    float3 resultColor = color.rgb * AMBIENT + color.rgb * light.Color * light.Intensity * saturate(dot(input.Normal, -lightDirection)) * saturate(shadowFactor);

    return float4(resultColor, 1.0f);
}
