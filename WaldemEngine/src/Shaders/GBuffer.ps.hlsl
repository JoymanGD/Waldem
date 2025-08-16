#include "Core.hlsl"
#include "Materials.hlsl"
#include "GBufferCommon.hlsl"

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
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

SamplerState myStaticSampler : register(s0);

float3 GetNormal(float3 normal, float3 tangent, float3 bitangent, float4 normalMap)
{
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return mul(normalMap.xyz * 2.0 - 1.0, TBN);
}

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[WorldTransforms];
    StructuredBuffer<MaterialAttribute> materialAttributes = ResourceDescriptorHeap[MaterialAttributes];

    MaterialAttribute matAttr = materialAttributes[MeshId];

    float4 color = input.Color * matAttr.Albedo;

    if(matAttr.DiffuseTextureIndex != -1)
    {
        Texture2D<float4> ColorTexture = ResourceDescriptorHeap[NonUniformResourceIndex(matAttr.DiffuseTextureIndex)];
        float4 sampledColor = ColorTexture.Sample(myStaticSampler, input.UV);
        
        if(sampledColor.a < 0.1f)
            discard;
        
        color *= sampledColor;
    }
    
    float4 normal = float4(0.0f, 1.0f, 0.0f, 1.0f);

    if(matAttr.NormalTextureIndex != -1)
    {
        Texture2D<float4> NormalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(matAttr.NormalTextureIndex)]; 
        normal = NormalTexture.Sample(myStaticSampler, input.UV);
        normal = float4(GetNormal(input.Normal, input.Tangent, input.Bitangent, normal), 0.0f);
    }
    else
    {
        normal = float4(input.Normal, 0.0f);
    }

    float4 orm = float4(0.0f, matAttr.Roughness, matAttr.Metallic, 0.0f);
    
    if(matAttr.ORMTextureIndex != -1)
    {
        Texture2D<float4> ORMTexture = ResourceDescriptorHeap[NonUniformResourceIndex(matAttr.ORMTextureIndex)];
        orm *= ORMTexture.Sample(myStaticSampler, input.UV);
    }
    
    output.ColorRT = color;
    output.NormalRT = normalize(mul(worldTransforms[MeshId], normal));
    output.WorldPositionRT = input.WorldPosition;
    output.ORM = orm;
    output.MeshIDRT = MeshId+1;
    return output;
}
