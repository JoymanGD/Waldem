struct VS_INPUT
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

cbuffer RootConstants : register(b0)
{
    matrix ViewProjection;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.Position = mul(ViewProjection, input.Position);
    output.Color = input.Color;

    return output;
}