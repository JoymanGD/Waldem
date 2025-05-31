struct Buffers
{
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
    uint WorldTransforms;
    uint MaterialAttributes;
    uint SceneDataBufferId;
};

cbuffer IndirectRootConstants : register(b1)
{
    uint MeshId;
};