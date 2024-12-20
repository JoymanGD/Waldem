#include "Lighting.hlsl"
#include "Shadows.hlsl"

SamplerState myStaticSampler : register(s0);
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D WorldPosition : register(t2);
Texture2D Normal : register(t3);
Texture2D Albedo : register(t4);
RWTexture2D<float4> DeferredRenderingRenderTarget : register(u0);

cbuffer MyConstantBuffer : register(b0)
{
    float2 Resolution;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float2 UV = (float2)tid / Resolution;
    float4 worldPosition = WorldPosition.SampleLevel(myStaticSampler, UV, 0);
    float3 normal = Normal.SampleLevel(myStaticSampler, UV, 0).xyz;
    float4 albedo = Albedo.SampleLevel(myStaticSampler, UV, 0);

    //Combining the lighting and shadowing
    Light light = Lights[0];

    float3 lightDirection = -GetLightDirection(light);
    
    float shadowFactor = CalculateShadowFactor(Shadowmap, cmpSampler, worldPosition, normal, light.View, light.Projection);

    float3 resultColor = albedo.rgb * AMBIENT + albedo.rgb * light.Color * light.Intensity * saturate(dot(normal, lightDirection)) * saturate(shadowFactor);

    //Writing the result to the render target
    DeferredRenderingRenderTarget[tid] = float4(resultColor, 1.0f);
}