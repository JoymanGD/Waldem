#include "Lighting.hlsl"
#include "Shadows.hlsl"

SamplerState myStaticSampler : register(s0);

Texture2D DeferredRenderingRenderTarget : register(t0);
RWTexture2D<float4> PostProcessRenderTarget : register(u0);

cbuffer MyConstantBuffer : register(b0)
{
    float2 Resolution;
};

cbuffer BloomParams : register(b1)
{
    float BrightThreshold;  // Brightness threshold
    float BloomIntensity;   // Intensity of the bloom effect
    float2 TexelSize;       // Size of one pixel in texture coordinates
};

float4 BloomEffect(float2 uv)
{
    // 1. Bright Pass: Extract bright regions
    float4 color = DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, uv, 0);
    float brightness = max(max(color.r, color.g), color.b);
    float4 brightColor = brightness > BrightThreshold ? color : float4(0, 0, 0, 0);

    // 2. Gaussian Blur (dual filtering)
    float4 blurredColor = float4(0, 0, 0, 0);
    // float weights[10] = { 0.227, 0.194, 0.121, 0.054, 0.016, 0.009, 0.004, 0.002, 0.001, 0.0005 };
    float weights[5] = { 0.227, 0.194, 0.121, 0.054, 0.016 };

    // Horizontal and Vertical blur in one pass
    for (int i = 0; i < 5; ++i)
    {
        float2 offsetH = float2(TexelSize.x * i, 0);
        float2 offsetV = float2(0, TexelSize.y * i);

        blurredColor += DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, uv + offsetH, 0) * weights[i];
        blurredColor += DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, uv - offsetH, 0) * weights[i];
        blurredColor += DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, uv + offsetV, 0) * weights[i];
        blurredColor += DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, uv - offsetV, 0) * weights[i];
    }

    return blurredColor;
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float2 UV = (float2)tid / Resolution;
    
    // // Compute the bloom effect
    // float4 bloom = BloomEffect(UV);
    //
    // // Combine bloom with the original scene
    // float4 originalColor = DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, UV, 0);
    // float4 finalColor = originalColor + bloom * BloomIntensity;
    //
    // //Writing the result to the render target
    // PostProcessRenderTarget[tid] = finalColor;
    
    PostProcessRenderTarget[tid] = DeferredRenderingRenderTarget.SampleLevel(myStaticSampler, UV, 0);
}