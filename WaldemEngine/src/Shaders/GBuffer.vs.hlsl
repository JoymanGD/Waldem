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

cbuffer RootConstants : register(b0)
{
    uint MeshId;
};

cbuffer MyConstantBuffer : register(b1)
{
    matrix view;
    matrix proj;
    matrix invView;
    matrix invProj;
};

StructuredBuffer<float4x4> WorldTransforms : register(t0);

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.WorldPosition = mul(WorldTransforms[MeshId], float4(input.Position, 1));
    output.Position = mul(view, output.WorldPosition);
    output.Position = mul(proj, output.Position);
    // output.Normal = normalize(mul(WorldTransforms[MeshId], float4(input.Normal, 0)).xyz);
    // output.Tangent = normalize(mul(WorldTransforms[MeshId], float4(input.Tangent, 0)).xyz);
    // output.Bitangent = normalize(mul(WorldTransforms[MeshId], float4(input.Bitangent, 0)).xyz);
    output.Normal = input.Normal;
    output.Tangent = input.Tangent;
    output.Bitangent = input.Bitangent;
    output.UV = input.UV;
    output.MeshId = input.MeshId;

    return output;
}