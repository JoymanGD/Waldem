struct TerrainSceneData
{
    float4x4 View;
    float4x4 Proj;
    float4x4 InvProj;
    float4x4 WorldTransform;
    uint HeightMapIndex;
    uint AlbedoIndex;
    int Resolution;
    float Height;
    uint VertexBufferIndex;
    uint IndexBufferIndex;
    uint PhysXSamplesBufferIndex;
};

struct TerrainVertex
{
    float3 Position;
    float3 Normal;
    float2 UV;
};

cbuffer RootConstants : register(b0)
{
    uint SceneDataBufferId;
};

#define DEFAULT_TERRAIN_SIZE 100.0f