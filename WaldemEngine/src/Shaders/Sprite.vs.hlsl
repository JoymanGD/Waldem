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
    uint SpriteIds;
    uint SceneDataBufferId;
};

cbuffer IndirectRootConstants : register(b1)
{
    uint MeshId;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
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
    output.Color = input.Color;
    output.Normal = float3(0,0,1);
    output.UV = input.UV;
    output.MeshId = MeshId;

    return output;
}