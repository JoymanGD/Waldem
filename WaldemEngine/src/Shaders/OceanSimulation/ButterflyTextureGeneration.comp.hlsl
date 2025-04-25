#include "../Complex.hlsl"
#define PI 3.14159265359f

StructuredBuffer<int> bitReversedIndices : register(t0);
RWTexture2D<float4> ButterflyTexture512 : register(u0);

cbuffer MyConstantBuffer : register(b0)
{
    uint N; // Size of the grid (e.g., 256)
}

[numthreads(1, 16, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float k = fmod(tid.y * (float(N) / pow(2, tid.x+1)), N);
    float kN = 2.0f*PI*k/float(N);
    complex twiddle = ccreate(cos(kN), sin(kN));

    int butterflySpan = int(pow(2, tid.x));
    int butterflyWing = fmod(tid.y, pow(2, tid.x + 1)) < pow(2, tid.x) ? 1 : 0;

    if(tid.x == 0)
    {
        if(butterflyWing == 1)
        {
            ButterflyTexture512[tid] = float4(twiddle.real, twiddle.imag, bitReversedIndices[tid.y], bitReversedIndices[tid.y + 1]);
        }
        else
        {
            ButterflyTexture512[tid] = float4(twiddle.real, twiddle.imag, bitReversedIndices[tid.y - 1], bitReversedIndices[tid.y]);
        }
    }
    else
    {
        if(butterflyWing == 1)
        {
            ButterflyTexture512[tid] = float4(twiddle.real, twiddle.imag, tid.y, tid.y + butterflySpan);
        }
        else
        {
            ButterflyTexture512[tid] = float4(twiddle.real, twiddle.imag, tid.y - butterflySpan, tid.y);
        }
    }
}