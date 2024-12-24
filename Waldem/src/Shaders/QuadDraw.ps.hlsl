struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

SamplerState myStaticSampler : register(s0);
Texture2D PostProcessRenderTarget : register(t0);

float4 main(PS_INPUT input) : SV_TARGET
{
    return PostProcessRenderTarget.Sample(myStaticSampler, input.UV);
}
