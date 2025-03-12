struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

SamplerState myStaticSampler : register(s0);
Texture2D TargetRT : register(t0);

float4 main(PS_INPUT input) : SV_TARGET
{
    return TargetRT.Sample(myStaticSampler, input.UV);
}
