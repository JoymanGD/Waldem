#include "Core.hlsl"

float CalculateShadowFactor(Texture2D<float> Shadowmap, SamplerComparisonState cmpSampler, float3 worldPos, float4x4 view, float4x4 projection)
{
    float4 lightClipPos = mul(view, float4(worldPos, 1.0f));
    lightClipPos = mul(projection, lightClipPos);

    lightClipPos.xyz /= lightClipPos.w;
    
    float2 shadowUV = lightClipPos.xy * 0.5f + 0.5f;
    shadowUV.y = -shadowUV.y;

    float depth = lightClipPos.z - SHADOW_BIAS;

    float sum = 0;
    
    float2 shadowMapSize = float2(2048, 2048);
    float xOffset = 1.0/shadowMapSize.x;
    float yOffset = 1.0/shadowMapSize.y;
 
    //perform PCF filtering on a 4 x 4 texel neighborhood
    for (int y = -1; y <= 1; y += 1)
    {
        for (int x = -1; x <= 1; x += 1)
        {
            float2 Offsets = float2(x * xOffset, y * yOffset);
            sum += Shadowmap.SampleCmpLevelZero(cmpSampler, shadowUV + Offsets, depth);
        }
    }

    return sum / 18.0f;
}