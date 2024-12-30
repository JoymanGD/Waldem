#define PI 3.14159265359f

SamplerState myStaticSampler : register(s0);

RWTexture2D<float4> SpectrumRenderTarget : register(u0);
RWTexture2D<float4> HeightmapRenderTarget : register(u1);
Texture2D GaussianNoiseRenderTarget : register(t0);

cbuffer MyConstantBuffer : register(b0)
{
    float2 Resolution;
}

float PhillipsSpectrum(float2 k, float kM)
{
    // Compute the Phillips spectrum
    float A = 10.f;  // Amplitude scaling factor (tuned based on wind speed)
	float V = 100.0f; // Wind speed
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

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
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

    SpectrumRenderTarget[tid] = float4(spectrum, spectrum, 0.0f, 0.0f);

    float4 noise = GaussianNoiseRenderTarget[tid];
	float halfSpectrumSqrt = sqrt(spectrum/2.0f);
    noise.x *= halfSpectrumSqrt;
    noise.y *= halfSpectrumSqrt;
    
    HeightmapRenderTarget[tid] = noise; 
}