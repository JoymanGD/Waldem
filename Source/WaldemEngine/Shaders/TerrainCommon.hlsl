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
};

cbuffer RootConstants : register(b0)
{
    uint SceneDataBufferId;
};