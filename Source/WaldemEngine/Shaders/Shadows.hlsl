#define SHADOW_BIAS 0.005f
#define NORMAL_OFFSET_BIAS 0.3f

float CalculateShadowFactorShadowMap(Texture2D<float> Shadowmap, SamplerComparisonState cmpSampler, float4 worldPos, float3 normal, float4x4 view, float4x4 projection)
{
    float4 worldPosFixed = worldPos + float4(normal, 0) * NORMAL_OFFSET_BIAS; //Normal offset bias to fix shadow acne
    float4 lightClipPos = mul(view, worldPosFixed); //view space
    lightClipPos = mul(projection, lightClipPos); //clip space
    lightClipPos.xyz /= lightClipPos.w; //NDC
    
    float2 shadowUV = lightClipPos.xy * 0.5f + 0.5f;
    shadowUV.y = -shadowUV.y;

    float depth = lightClipPos.z;

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