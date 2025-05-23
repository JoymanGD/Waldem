#include "Core.hlsl"
#include "Materials.hlsl"
#include "GBufferCommon.hlsl"

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

SamplerState myStaticSampler : register(s0);

float3 GetNormal(float3 normal, float3 tangent, float3 bitangent, float4 normalMap)
{
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return mul(normalMap.xyz * 2.0 - 1.0, TBN);
}

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    StructuredBuffer<Buffers> buffersBuffer = ResourceDescriptorHeap[BuffersBufferId];
    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[buffersBuffer[0].WorldTransforms];
    StructuredBuffer<MaterialAttribute> materialAttributes = ResourceDescriptorHeap[buffersBuffer[0].MaterialAttributes];

    MaterialAttribute matAttr = materialAttributes[MeshId];
    float4 color, normal, orm;

    if(matAttr.DiffuseTextureIndex != -1)
    {
        Texture2D ColorTexture = ResourceDescriptorHeap[matAttr.DiffuseTextureIndex];
        color = ColorTexture.Sample(myStaticSampler, input.UV);
        
        if(color.a < 0.1f)
            discard;
    }
    else
    {
        color = matAttr.Albedo;
    }

    if(matAttr.NormalTextureIndex != -1)
    {
        Texture2D NormalTexture = ResourceDescriptorHeap[matAttr.NormalTextureIndex];
        normal = NormalTexture.Sample(myStaticSampler, input.UV);
        normal = float4(GetNormal(input.Normal, input.Tangent, input.Bitangent, normal), 0.0f);
    }
    else
    {
        normal = float4(input.Normal, 0.0f);
    }

    if(matAttr.ORMTextureIndex != -1)
    {
        Texture2D ORMTexture = ResourceDescriptorHeap[matAttr.ORMTextureIndex];
        orm = ORMTexture.Sample(myStaticSampler, input.UV);
    }
    else
    {
        orm = float4(0.0f, matAttr.Roughness, matAttr.Metallic, 0.0f);
    }
    
    output.ColorRT = color;
    output.NormalRT = normalize(mul(worldTransforms[MeshId], normal));
    output.WorldPositionRT = input.WorldPosition;
    output.ORM = orm;
    output.MeshIDRT = MeshId+1;

    return output;
}
