struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
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
    
    output.Position = mul(WorldTransforms[MeshId], float4(input.Position, 1));
    output.Position = mul(view, output.Position);
    output.Position = mul(proj, output.Position);

    return output;
}