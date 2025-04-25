struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

cbuffer RootConstants : register(b0)
{
    matrix ViewProjection;
    float4 Color;
};

float4 main() : SV_TARGET
{
    return Color;
}
