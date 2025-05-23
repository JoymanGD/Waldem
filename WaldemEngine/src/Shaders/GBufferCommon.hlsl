struct Buffers
{
    uint WorldTransforms;
    uint MaterialAttributes;
};

struct SceneData
{
    float4x4 View;
    float4x4 Proj;
    float4x4 InvView;
    float4x4 InvProj;
};

cbuffer RootConstants : register(b0)
{
    uint BuffersBufferId;
    uint SceneDataBufferId;
};

cbuffer IndirectRootConstants : register(b1)
{
    uint MeshId;
};