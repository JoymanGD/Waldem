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
    matrix world;
    matrix viewProj;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    output.Position = mul(world, float4(input.Position, 1));
    output.Position = mul(viewProj, output.Position);

    return output;
}