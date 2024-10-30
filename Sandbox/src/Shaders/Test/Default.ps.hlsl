struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = float3(input.UV, 0.5);

    return float4(color, 1.0);
}
