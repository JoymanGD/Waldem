#include "../Complex.hlsl"
#include "OceanResources.hlsl"

#define PI 3.14159265359f

Texture2D GaussianNoiseRenderTarget : register(t0);
RWTexture2D<float4> Ho : register(u0);
RWTexture2D<float4> HoInverse : register(u1);
RWTexture2D<float4> HDx : register(u2);
RWTexture2D<float4> HDy : register(u3);
RWTexture2D<float4> HDz : register(u4);

cbuffer MyPushConstants : register(b1)
{
	float t;
}

float PhillipsSpectrum(float2 k, float kM)
{
    // Compute the Phillips spectrum
    float L = V*V/G;  // Large-scale wave length parameter (dependent on the environment)
	float L2 = L*L;
	float l2 = Damping * Damping;
    float kML2 = kM * kM * L2;
	float kW = dot(normalize(k), normalize(W));
	float kW2 = kW*kW;
	float kW6 = kW2*kW2*kW2;
	float kMl2 = kM*kM*l2;
	float kM4 = kM * kM * kM * kM;
    // float spectrum = A / kM4 * kW4 * exp(-1.0f / kML2) * exp(-kMl2);
    float spectrum = A * exp(-1.0f / kML2) / kM4 * kW6 * exp(-kMl2);
    return spectrum;
}

void GenerateInitialSpectrum(uint2 tid, out float4 h0, out float4 h0Inverse)
{
	float2 coords = tid - float(N)/2.0f;

	float2 k = 2*PI*coords/L;
	float kM = max(length(k), 0.0001f);

	float spectrum = PhillipsSpectrum(k, kM);
	float spectrumInverse = PhillipsSpectrum(-k, kM);

	float4 noise = GaussianNoiseRenderTarget[tid];
	
	float halfSpectrumSqrt = clamp(sqrt(spectrum)/sqrt(2.0f), -4000.f, 140000.f);
	float halfSpectrumInverseSqrt = clamp(sqrt(spectrumInverse)/sqrt(2.0f), -4000.f, 140000.f);
    
	h0 = float4(noise.xy * halfSpectrumSqrt, 0, 1);
	h0Inverse = float4(noise.zw * halfSpectrumInverseSqrt, 0, 1);
}

void GenerateAxesSpectrums(uint2 tid, float2 h0, float2 h0Inverse, out float4 hDx, out float4 hDy, out float4 hDz)
{
	float2 coords = tid - float(N)/2.0f;

	float2 k = 2*PI*coords/L;
	float kM = max(length(k), 0.0001f);
	
	float w = sqrt(G * kM);

	complex fourier = ccreate(h0.x, h0.y);
	complex fourierConj = cconj(ccreate(h0Inverse.x, h0Inverse.y));

	float cosWt = cos(w * t * SimulationSpeed);
	float sinWt = sin(w * t * SimulationSpeed);

	//euler formula
	complex e = ccreate(cosWt, sinWt);
	complex eInv = ccreate(cosWt, -sinWt);

	//dy
	complex dy = cadd(cmul(fourier, e), cmul(fourierConj, eInv));

	//dx
	complex dx = cmul(ccreate(0.0f, -k.x/kM), dy);

	//dz
	complex dz = cmul(ccreate(0.0f, -k.y/kM), dy);

	hDy = float4(dy.real, dy.imag, 0, 1);
	hDx = float4(dx.real, dx.imag, 0, 1);
	hDz = float4(dz.real, dz.imag, 0, 1);
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
	float4 h0, h0Inverse;
	GenerateInitialSpectrum(tid, h0, h0Inverse);
	Ho[tid] = h0;
	HoInverse[tid] = h0Inverse;

	float4 hDx, hDy, hDz;
	GenerateAxesSpectrums(tid, h0.xy, h0Inverse.xy, hDx, hDy, hDz);
	HDx[tid] = hDx;
	HDy[tid] = hDy;
	HDz[tid] = hDz;
}