struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
    uint VertexID : SV_VertexID;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

cbuffer MyConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
};

cbuffer RootConstants : register(b1)
{
    uint MeshId;
};

StructuredBuffer<float4x4> WorldTransforms : register(t0);

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.WorldPosition = mul(WorldTransforms[MeshId], float4(input.Position, 1));
    output.Position = mul(view, output.WorldPosition);
    output.Position = mul(proj, output.Position);
    output.Normal = normalize(mul(WorldTransforms[MeshId], float4(input.Normal, 0)).xyz);
    output.UV = input.UV;
    output.MeshId = MeshId;

    return output;
}