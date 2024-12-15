#include "Core.hlsl"

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct PS_OUTPUT
{
    float4 WorldPosition : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Albedo : SV_TARGET2;
};

SamplerState myStaticSampler : register(s0);

Texture2D DiffuseTextures[MAX_TEXTURES] : register(t1);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);
    
    if(color.a < 0.1f)
        discard;

    output.WorldPosition = input.WorldPosition;
    output.Normal = float4(input.Normal, 1.0f);
    output.Albedo = color;

    return output;
}
