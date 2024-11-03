struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD;
};

// Buffer<float3> TestBuffer : register(t0);

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = float3(input.UV, 0.5);
    // float3 color = TestBuffer[0];

    return float4(color, 1.0);
}
