RWTexture2D<float> TestRenderTarget : register(u0);
SamplerState myStaticSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    TestRenderTarget[tid] = .35f;
}