#include "../Complex.hlsl"

Texture2D ButterflyTexture512 : register(t0);
RWTexture2D<float4> PingPong0 : register(u0);
RWTexture2D<float4> PingPong1 : register(u1);

cbuffer MyPushConstants : register(b0)
{
    uint stage;
    uint pingpong;
	uint direction;
}

void Horizontal(uint2 tid)
{
    if(pingpong == 0)
    {
        float4 data = ButterflyTexture512.Load(uint3(stage, tid.x, 0));
        float2 first = PingPong0.Load(uint3(data.z, tid.y, 0)).xy;
        float2 second = PingPong0.Load(uint3(data.w, tid.y, 0)).xy;
        complex firstComplex = ccreate(first.x, first.y);
        complex secondComplex = ccreate(second.x, second.y);
        complex twiddle = ccreate(data.x, data.y);

        complex secondTwiddled = cmul(twiddle, secondComplex);
        complex H = cadd(firstComplex, secondTwiddled);
        PingPong1[tid] = float4(H.real, H.imag, 0, 1);
    }
    else if(pingpong == 1)
    {
        float4 data = ButterflyTexture512.Load(uint3(stage, tid.x, 0));
        float2 first = PingPong1.Load(uint3(data.z, tid.y, 0)).xy;
        float2 second = PingPong1.Load(uint3(data.w, tid.y, 0)).xy;
        complex firstComplex = ccreate(first.x, first.y);
        complex secondComplex = ccreate(second.x, second.y);
        complex twiddle = ccreate(data.x, data.y);

        complex secondTwiddled = cmul(twiddle, secondComplex);
        complex H = cadd(firstComplex, secondTwiddled);
        PingPong0[tid] = float4(H.real, H.imag, 0, 1);
    }
}

void Vertical(uint2 tid)
{
    if(pingpong == 0)
    {
        float4 data = ButterflyTexture512.Load(uint3(stage, tid.y, 0));
        float2 first = PingPong0.Load(uint3(tid.x, data.z, 0)).xy;
        float2 second = PingPong0.Load(uint3(tid.x, data.w, 0)).xy;
        complex firstComplex = ccreate(first.x, first.y);
        complex secondComplex = ccreate(second.x, second.y);
        complex twiddle = ccreate(data.x, data.y);

        complex secondTwiddled = cmul(twiddle, secondComplex);
        complex H = cadd(firstComplex, secondTwiddled);
        PingPong1[tid] = float4(H.real, H.imag, 0, 1);
    }
    else if(pingpong == 1)
    {
        float4 data = ButterflyTexture512.Load(uint3(stage, tid.y, 0));
        float2 first = PingPong1.Load(uint3(tid.x, data.z, 0)).xy;
        float2 second = PingPong1.Load(uint3(tid.x, data.w, 0)).xy;
        complex firstComplex = ccreate(first.x, first.y);
        complex secondComplex = ccreate(second.x, second.y);
        complex twiddle = ccreate(data.x, data.y);

        complex secondTwiddled = cmul(twiddle, secondComplex);
        complex H = cadd(firstComplex, secondTwiddled);
        PingPong0[tid] = float4(H.real, H.imag, 0, 1);
    }
}

[numthreads(16, 16, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    if(direction == 0)
        Horizontal(tid);
    else if(direction == 1)
        Vertical(tid);
}