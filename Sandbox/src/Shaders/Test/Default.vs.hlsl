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

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.Position = float4(input.Position, 10);
    output.Normal = input.Normal;
    output.UV = input.UV;

    return output;
}