SamplerState myStaticSampler : register(s0);

Texture2D Dx : register(t0);
Texture2D Dy : register(t1);
Texture2D Dz : register(t2);

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
}