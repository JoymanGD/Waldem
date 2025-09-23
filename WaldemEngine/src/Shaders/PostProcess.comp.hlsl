struct BloomPostProcess
{
    float BrightThreshold;
    float BloomIntensity;
};

cbuffer RootConstants : register(b0)
{
    uint SrcRTID;
    uint DstRTID;
    uint BloomParamsBuffer;
    uint BlurDirection;
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

[numthreads(8,8,1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    Texture2D src = ResourceDescriptorHeap[SrcRTID];
    RWTexture2D<float4> dst = ResourceDescriptorHeap[DstRTID];
    StructuredBuffer<BloomPostProcess> bloomParamsBuffer = ResourceDescriptorHeap[BloomParamsBuffer];
    BloomPostProcess bloomParams = bloomParamsBuffer[0];

    float4 result = float4(0,0,0,1);

    if (BlurDirection == 0) // bright-pass
    {
        result = BrightPass(tid, src, bloomParams.BrightThreshold);
    }
    else if (BlurDirection == 1) // horizontal blur
    {
        result = BlurPass(tid, src, 1);
    }
    else if (BlurDirection == 2) // vertical blur
    {
        result = BlurPass(tid, src, 2) * bloomParams.BloomIntensity;
    }
    else if (BlurDirection == 3) // final composite
    {
        float4 original = dst.Load(uint3(tid, 0));
        float4 bloom = src.Load(uint3(tid, 0));
        result = original + bloom;
        result.a = 1.0;
    }

    dst[tid] = result;
}