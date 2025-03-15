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
};

cbuffer RootConstants : register(b0)
{
    matrix WorldViewProj;
    float4 Color;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.Position = mul(WorldViewProj, float4(input.Position, 1));

    return output;
}