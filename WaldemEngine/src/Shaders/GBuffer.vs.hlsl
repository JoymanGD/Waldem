#include "GBufferCommon.hlsl"

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

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

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    StructuredBuffer<SceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferId];
    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[WorldTransforms];
    SceneData sceneData = sceneDataBuffer[0];
    
    output.WorldPosition = mul(worldTransforms[MeshId], float4(input.Position, 1));
    output.Position = mul(sceneData.View, output.WorldPosition);
    output.Position = mul(sceneData.Proj, output.Position);
    // output.Normal = normalize(mul(WorldTransforms[MeshId], float4(input.Normal, 0)).xyz);
    // output.Tangent = normalize(mul(WorldTransforms[MeshId], float4(input.Tangent, 0)).xyz);
    // output.Bitangent = normalize(mul(WorldTransforms[MeshId], float4(input.Bitangent, 0)).xyz);
    output.Normal = input.Normal;
    output.Tangent = input.Tangent;
    output.Bitangent = input.Bitangent;
    output.UV = input.UV;
    output.MeshId = MeshId;

    return output;
}