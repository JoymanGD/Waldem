struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
    float  Alpha    : TEXCOORD1;
    float4 Color    : COLOR;
};

float4 main(PSInput pin) : SV_Target
{
    float2 centeredUV = pin.UV * 2.0f - 1.0f;
    float dist = length(centeredUV);
    float radialFade = saturate(1.0f - dist);

    float alpha = pin.Alpha * radialFade;

    return float4(pin.Color.rgb, pin.Color.a * alpha);
}