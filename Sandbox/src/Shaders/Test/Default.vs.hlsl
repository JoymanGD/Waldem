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
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

cbuffer MyConstantBuffer : register(b0)
{
    matrix vp;
    matrix world;
    uint lightsAmount;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 position = mul(world, float4(input.Position, 1));
    output.Position = mul(vp, position);
    output.Normal = input.Normal;
    output.UV = input.UV;
    output.MeshId = input.MeshId;

    return output;
}