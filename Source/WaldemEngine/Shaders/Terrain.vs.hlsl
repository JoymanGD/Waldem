#include "TerrainCommon.hlsl"

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 UV : TEXCOORD;
};

struct VS_OUTPUT
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

SamplerState myStaticSampler : register(s0);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    StructuredBuffer<TerrainSceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferId];
    TerrainSceneData sceneData = sceneDataBuffer[0];

    float3 localPos = input.Position;

    float3 normal = input.Normal;
    
    if(sceneData.HeightMapIndex > 0)
    {
        //displacement
        Texture2D<float> heightMap = ResourceDescriptorHeap[NonUniformResourceIndex(sceneData.HeightMapIndex)];
        float height01 = heightMap.SampleLevel(myStaticSampler, input.UV, 0.0f).r;
        localPos.y = height01 * sceneData.Height;

        //normals
        uint width, height;
        heightMap.GetDimensions(width, height);
        float2 texelSize = 1.0 / float2(width, height);
        float hL = heightMap.SampleLevel(myStaticSampler, input.UV - float2(texelSize.x, 0.0), 0.0).r * sceneData.Height;
        float hR = heightMap.SampleLevel(myStaticSampler, input.UV + float2(texelSize.x, 0.0), 0.0).r * sceneData.Height;
        float hD = heightMap.SampleLevel(myStaticSampler, input.UV - float2(0.0, texelSize.y), 0.0).r * sceneData.Height;
        float hU = heightMap.SampleLevel(myStaticSampler, input.UV + float2(0.0, texelSize.y), 0.0).r * sceneData.Height;
        float terrainSize = 100.0;
        float stepX = terrainSize * texelSize.x;
        float stepZ = terrainSize * texelSize.y;
        float3 dx = float3(2.0 * stepX, hR - hL, 0.0);
        float3 dz = float3(0.0, hU - hD, 2.0 * stepZ);
        normal = normalize(cross(dz, dx));
    }
    
    output.WorldPosition = mul(sceneData.WorldTransform, float4(localPos, 1));
    output.Position = mul(sceneData.View, output.WorldPosition);
    output.Position = mul(sceneData.Proj, output.Position);
    // output.Normal = normalize(mul(WorldTransforms[MeshId], float4(input.Normal, 0)).xyz);
    // output.Tangent = normalize(mul(WorldTransforms[MeshId], float4(input.Tangent, 0)).xyz);
    // output.Bitangent = normalize(mul(WorldTransforms[MeshId], float4(input.Bitangent, 0)).xyz);
    output.Color = input.Color;
    output.Normal = normal;
    output.Tangent = input.Tangent;
    output.Bitangent = input.Bitangent;
    output.UV = input.UV;
    output.MeshId = 999999;

    return output;
}