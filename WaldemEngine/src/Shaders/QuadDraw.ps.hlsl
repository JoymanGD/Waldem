#pragma shader_model 6_6

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

SamplerState myStaticSampler : register(s0);

cbuffer ScreenQuadRootConstants : register(b0)
{
    uint TargetRTId;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    Texture2D TargetRT = ResourceDescriptorHeap[TargetRTId];
    return TargetRT.Sample(myStaticSampler, input.UV);
}