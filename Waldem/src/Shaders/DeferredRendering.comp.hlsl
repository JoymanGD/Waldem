#include "Lighting.hlsl"
#include "Shadows.hlsl"

SamplerState myStaticSampler : register(s0);
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D WorldPositionRT : register(t2);
Texture2D NormalRT : register(t3);
Texture2D AlbedoRT : register(t4);
Texture2D MeshIDRT : register(t5);
Texture2D DepthRT : register(t6);
RWTexture2D<float4> DeferredRenderingRenderTarget : register(u0);
RWStructuredBuffer<int> HoveredMeshes : register(u1);

cbuffer MyConstantBuffer : register(b0)
{
    float2 Resolution;
};

cbuffer RootConstants : register(b1)
{
    float2 MousePosition;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float2 UV = (float2)tid / Resolution;
    float4 worldPosition = WorldPositionRT.SampleLevel(myStaticSampler, UV, 0);
    float3 normal = NormalRT.SampleLevel(myStaticSampler, UV, 0).xyz;
    float4 albedo = AlbedoRT.SampleLevel(myStaticSampler, UV, 0);

    float2 mousePosUV = MousePosition / Resolution;
    float depthMousePos = DepthRT.SampleLevel(myStaticSampler, mousePosUV, 0).x;
    if(depthMousePos > 0.999f) //sky
    {
        HoveredMeshes[0] = -1;
    }
    else
    {
        HoveredMeshes[0] = asint(MeshIDRT.SampleLevel(myStaticSampler, mousePosUV, 0).x);
    }
    
    //Combining the lighting and shadowing
    Light light = Lights[0];

    float3 lightDirection = -GetLightDirection(light);
    
    float shadowFactor = CalculateShadowFactor(Shadowmap, cmpSampler, worldPosition, normal, light.View, light.Projection);

    float3 resultColor = albedo.rgb * AMBIENT + albedo.rgb * light.Color * light.Intensity * saturate(dot(normal, lightDirection)) * saturate(shadowFactor);

    //Writing the result to the render target
    DeferredRenderingRenderTarget[tid] = float4(resultColor, 1.0f);
}