#include "TerrainCommon.hlsl"

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

SamplerState myStaticSampler : register(s0);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    StructuredBuffer<TerrainSceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferId];
    TerrainSceneData sceneData = sceneDataBuffer[0];

    output.WorldPosition = mul(sceneData.WorldTransform, float4(input.Position, 1));
    output.Position = mul(sceneData.View, output.WorldPosition);
    output.Position = mul(sceneData.Proj, output.Position);
    output.Normal = input.Normal;
    output.UV = input.UV;
    output.MeshId = 999999;

    return output;
}