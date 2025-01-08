#ifdef _DXC_COMPILER
	#include "Complex.hlsl"
#else
	#include "../Complex.hlsl"
#endif

#define PI 3.14159265359f

Texture2D GaussianNoiseRenderTarget : register(t0);
RWTexture2D<float4> Ho : register(u0);
RWTexture2D<float4> HoInverse : register(u1);
RWTexture2D<float4> HDx : register(u2);
RWTexture2D<float4> HDy : register(u3);
RWTexture2D<float4> HDz : register(u4);

cbuffer MyConstantBuffer : register(b0)
{
	float2 Resolution;
	uint N;
}

cbuffer MyPushConstants : register(b1)
{
	float t;
}

float PhillipsSpectrum(float2 k, float kM)
{
    // Compute the Phillips spectrum
    float A = 4.f;  // Amplitude scaling factor (tuned based on wind speed)
	float V = 40.0f; // Wind speed
	float2 W = float2(1.f, 1.f); // Wind direction
	float g = 9.80665f; // Gravitational constant
    float L = V*V/g;  // Large-scale wave length parameter (dependent on the environment)
	float L2 = L*L;
	float damping = 0.001f;
	float l2 = L2 * damping * damping;
    float kML2 = kM * kM * L2;
	float kW = dot(normalize(k), normalize(W));
	float kW2 = kW*kW;
	float kMl2 = kM*kM*l2;
	float kM4 = kM * kM * kM * kM;
    float spectrum = A / kM4 * kW2 * exp(-1.0f / kML2) * exp(-kMl2);
    return spectrum;
}

void GenerateInitialSpectrum(uint2 tid, out float4 h0, out float4 h0Inverse)
{
	int x = tid.x;
	int z = tid.y;

	int gridSize = Resolution.x;
    
	// Compute the wave vector components (kx, kz)
	float kx = (2 * PI * (x - gridSize / 2)) / gridSize;
	float kz = (2 * PI * (z - gridSize / 2)) / gridSize;
    
	// Compute the magnitude of the wave vector
	float2 k = float2(kx, kz);
	float kM = length(k);
    
	if (kM == 0.0f)
	{
		return;  // Skip for zero-frequency component (DC component)
	}

	float spectrum = PhillipsSpectrum(k, kM);
	float spectrumInverse = PhillipsSpectrum(-k, kM);

	float4 noise = GaussianNoiseRenderTarget[tid];
	
	float halfSpectrumSqrt = sqrt(spectrum/2.0f);
	float halfSpectrumInverseSqrt = sqrt(spectrumInverse/2.0f);
    
	h0 = float4(noise.x * halfSpectrumSqrt, noise.y * halfSpectrumSqrt, 0, 1);
	h0Inverse = float4(noise.x * halfSpectrumInverseSqrt, noise.y * halfSpectrumInverseSqrt, 0, 1);
}

void GenerateAxesSpectrums(uint2 tid, float2 h0, float2 h0Inverse, out float4 hDx, out float4 hDy, out float4 hDz)
{
	uint L = 1000;
	
	float2 coords = tid - float(N)/2.0f;
	float2 k = 2.0f * PI * coords / float(L);
	float kM = max(length(k), 0.0001f);
	float w = sqrt(9.81f * kM);

	complex fourier = { h0.x, h0.y };
	complex fourierConj = cconj(ccreate(h0Inverse.x, h0Inverse.y));

	float cosWt = cos(w * t);
	float sinWt = sin(w * t);

	//euler formula
	complex e = { cosWt, sinWt };
	complex eInv = { cosWt, -sinWt };

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