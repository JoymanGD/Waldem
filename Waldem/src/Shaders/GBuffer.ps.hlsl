#include "Core.hlsl"

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct PS_OUTPUT
{
    float4 WorldPositionRT : SV_TARGET0;
    float4 NormalRT : SV_TARGET1;
    float4 ColorRT : SV_TARGET2;
    float4 MetalRoughnessRT : SV_TARGET3;
    int MeshIDRT : SV_TARGET4;
};

cbuffer RootConstants : register(b1)
{
    uint MeshId;
};

SamplerState myStaticSampler : register(s0);

Texture2D DiffuseTextures[MAX_TEXTURES] : register(t1);
Texture2D NormalTextures[MAX_TEXTURES] : register(t1025);
Texture2D MetalRoughnessTextures[MAX_TEXTURES] : register(t2049);

float3 GetNormal(float3 normal, float3 tangent, float4 normalMap)
{
    float3 bitangent = cross(normal, tangent) * normalMap.w;
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return normalize(mul(normalMap.xyz * 2.0 - 1.0, TBN));
}

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 color = DiffuseTextures[MeshId].Sample(myStaticSampler, input.UV);
    float4 normalMap = NormalTextures[MeshId].Sample(myStaticSampler, input.UV);
    float4 metalRoughness = MetalRoughnessTextures[MeshId].Sample(myStaticSampler, input.UV);
    
    if(color.a < 0.1f)
        discard;

    output.WorldPositionRT = input.WorldPosition;
    output.NormalRT = float4(GetNormal(input.Normal, input.Tangent, normalMap), 1.0f);
    output.ColorRT = color;
    output.MetalRoughnessRT = metalRoughness;
    output.MeshIDRT = MeshId;

    return output;
}
