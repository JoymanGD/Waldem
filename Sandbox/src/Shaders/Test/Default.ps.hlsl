#include "Core.hlsl"

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct TestStruct
{
    float3 Color;
    float Intensity;
};

SamplerState myStaticSampler : register(s0);

Texture2D DiffuseTextures[MAX_TEXTURES] : register(t2);

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);

    if(color.a < 0.1f)
        discard;
    
    return float4(color.rgb, 1.0f);
}
