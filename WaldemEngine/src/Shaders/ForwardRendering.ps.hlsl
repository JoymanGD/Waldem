#include "Shading.hlsl"
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

Texture2D RadianceRT : register(t0);
Texture2D DiffuseTextures[] : register(t2);

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);
    
    if(color.a < 0.1f)
        discard;

    float3 radiance = RadianceRT.Sample(myStaticSampler, input.UV);

    float3 resultColor = color.rgb * AMBIENT + color.rgb * radiance;

    return float4(resultColor, 1.0f);
}
