struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD;
};

cbuffer MyConstantBuffer1 : register(b0)
{
    matrix vp;
};

cbuffer MyConstantBuffer2 : register(b1)
{
    matrix world;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 position = mul(world, float4(input.Position, 1));
    output.Position = mul(vp, position);
    output.Normal = input.Normal;
    output.UV = input.UV;

    return output;
}