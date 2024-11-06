struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD;
};

struct TestStruct
{
    float3 Color;
    float Intensity;
};

SamplerState myStaticSampler : register(s0);

Buffer<float3> TestBuffer : register(t0);
Buffer<TestStruct> TestBuffer2 : register(t1);
Texture2D DiffuseTextures[1024] : register(t2);

float4 main(PS_INPUT input) : SV_TARGET
{
    //float3 color = float3(input.UV, 0.5);
    float3 color = TestBuffer[0];
    color += TestBuffer2[0].Color * TestBuffer2[0].Intensity;
    color = DiffuseTextures[0].SampleLevel(myStaticSampler, input.UV, 0).xyz;
    return float4(color, 1.0);
}
