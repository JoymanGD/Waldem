struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.Position = float4(input.Position, 1.0f);
    output.UV = input.UV;

    return output;
}