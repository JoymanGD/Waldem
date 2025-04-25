#include "Core.hlsl"
#include "Materials.hlsl"

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct PS_OUTPUT
{
    float4 WorldPositionRT : SV_TARGET0;
    float4 NormalRT : SV_TARGET1;
    float4 ColorRT : SV_TARGET2;
    float4 ORM : SV_TARGET3;
    int MeshIDRT : SV_TARGET4;
};

cbuffer RootConstants : register(b1)
{
    uint MeshId;
};

SamplerState myStaticSampler : register(s0);

StructuredBuffer<float4x4> WorldTransforms : register(t0);
StructuredBuffer<MaterialAttribute> MaterialAttributes : register(t1);
Texture2D MaterialTextures[MAX_TEXTURES] : register(t2);

float3 GetNormal(float3 normal, float3 tangent, float3 bitangent, float4 normalMap)
{
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return mul(normalMap.xyz * 2.0 - 1.0, TBN);
}

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;

    MaterialAttribute matAttr = MaterialAttributes[MeshId];
    float4 color, normal, orm;

    if(matAttr.DiffuseTextureIndex != -1)
    {
        color = MaterialTextures[matAttr.DiffuseTextureIndex].Sample(myStaticSampler, input.UV);
        
        if(color.a < 0.1f)
            discard;
    }
    else
    {
        color = matAttr.Albedo;
    }

    if(matAttr.NormalTextureIndex != -1)
    {
        normal = MaterialTextures[matAttr.NormalTextureIndex].Sample(myStaticSampler, input.UV);
        normal = float4(GetNormal(input.Normal, input.Tangent, input.Bitangent, normal), 0.0f);
    }
    else
    {
        normal = float4(input.Normal, 0.0f);
    }

    if(matAttr.ORMTextureIndex != -1)
    {
        orm = MaterialTextures[matAttr.ORMTextureIndex].Sample(myStaticSampler, input.UV);
    }
    else
    {
        orm = float4(0.0f, matAttr.Roughness, matAttr.Metallic, 0.0f);
    }
    
    output.ColorRT = color;
    output.NormalRT = normalize(mul(WorldTransforms[MeshId], normal));
    output.WorldPositionRT = input.WorldPosition;
    output.ORM = orm;
    output.MeshIDRT = MeshId+1;

    return output;
}
