struct PostProcess
{
    float BrightThreshold;
    float BloomIntensity;
    float Exposure;
    float Saturation;
};

cbuffer RootConstants : register(b0)
{
    uint SrcRTID;
    uint DstRTID;
    uint PostProcessParamsBuffer;
    uint Stage;
}

// Bright-pass extraction
float4 BrightPass(int2 tid, Texture2D src, float threshold)
{
    float4 c = src.Load(uint3(tid, 0));
    float brightness = max(max(c.r, c.g), c.b);
    return (brightness > threshold) ? c : float4(0,0,0,0);
}

// Separable blur (horizontal or vertical)
float4 BlurPass(int2 tid, Texture2D src, uint dir)
{
    float weights[10] = {
        0.227, 0.194, 0.121, 0.054, 0.016,
        0.009, 0.004, 0.002, 0.001, 0.0005
    };

    float4 blurred = float4(0,0,0,0);

    for (int i = 0; i < 10; ++i)
    {
        int2 offset = (dir == 1) ? int2(i, 0) : int2(0, i);

        float4 c0 = src.Load(uint3(tid + offset, 0));
        float4 c1 = (i > 0) ? src.Load(uint3(tid - offset, 0)) : float4(0,0,0,0);

        blurred += (c0 + c1) * weights[i];
    }
    return blurred;
}

float3 TonemapACES(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

float3 AdjustSaturation(float3 color, float saturation)
{
    float gray = dot(color, float3(0.299, 0.587, 0.114)); // luminance
    return lerp(gray.xxx, color, saturation);
}

[numthreads(8,8,1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    Texture2D src = ResourceDescriptorHeap[SrcRTID];
    RWTexture2D<float4> dst = ResourceDescriptorHeap[DstRTID];
    StructuredBuffer<PostProcess> postProcessParamsBuffer = ResourceDescriptorHeap[PostProcessParamsBuffer];
    PostProcess postProcessParams = postProcessParamsBuffer[0];

    float4 result = float4(0,0,0,1);

    if (Stage == 0) // copy
    {
        result = src.Load(uint3(tid, 0));
    }
    else if (Stage == 1) // bright-pass
    {
        result = BrightPass(tid, src, postProcessParams.BrightThreshold);
    }
    else if (Stage == 2) // horizontal blur
    {
        result = BlurPass(tid, src, 1);
    }
    else if (Stage == 3) // vertical blur
    {
        result = BlurPass(tid, src, 2);
    }
    else if (Stage == 4) // final composite
    {
        float4 bloom = src.Load(uint3(tid, 0));
        float4 scene = dst.Load(uint3(tid, 0));
        result = scene + bloom * postProcessParams.BloomIntensity;
        result.a = 1.0;
    }
    else if (Stage == 5) // tonemap
    {
        float3 hdr = src.Load(uint3(tid, 0)).rgb;

        hdr *= postProcessParams.Exposure;

        float3 ldr = TonemapACES(hdr);

        ldr = pow(ldr, 1.0/2.2); // gamma correction
        ldr = AdjustSaturation(ldr, postProcessParams.Saturation); // e.g. boost by 20%

        result = float4(ldr, 1.0);
    }

    dst[tid] = result;
}