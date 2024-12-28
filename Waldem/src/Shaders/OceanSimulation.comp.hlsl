#define PI 3.14159265359f

SamplerState myStaticSampler : register(s0);

RWTexture2D<float4> SpectrumRenderTarget : register(u0);
RWTexture2D<float4> HeightmapRenderTarget : register(u1);

cbuffer MyConstantBuffer : register(b0)
{
    float2 Resolution;
}

float PhillipsSpectrum(float k)
{
    // Compute the Phillips spectrum
    float A = 1.0f;  // Amplitude scaling factor (tuned based on wind speed)
    float L = 1.0f;  // Large-scale wave length parameter (dependent on the environment)
    float kL = k * L;
    float spectrum = A * exp(-1.0f / (kL * kL)) / (k * k * k * k);
    return spectrum;
}

uint xorshift32(uint state)
{
    state ^= (state << 13);
    state ^= (state >> 17);
    state ^= (state << 5);
    return state;
}

uint lcg_rand(uint seed)
{
    return (1664525 * seed + 1013904223) % (1u << 31);  // A simple LCG
}

// Box-Muller transform to generate Gaussian random numbers
void GenerateGaussianRandom(uint3 dispatchThreadID, out float real, out float imag)
{
    uint seed = dispatchThreadID.x + dispatchThreadID.y * 12345 + dispatchThreadID.z * 67890;
    
    // Generate two uniform random numbers
    float u1 = (float)(lcg_rand(seed) % (1u << 16)) / (float)(1u << 16);
    float u2 = (float)(lcg_rand(seed + 1) % (1u << 16)) / (float)(1u << 16);

    // Apply the Box-Muller transform
    float z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.141592653589793 * u2);
    float z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * 3.141592653589793 * u2);

    // Return the Gaussian-distributed random numbers
    real = z0;
    imag = z1;
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    int x = tid.x;
    int z = tid.y;

    int gridSize = Resolution.x;
    
    // Compute the wave vector components (kx, kz)
    float kx = (2 * PI * (x - gridSize / 2)) / gridSize;
    float kz = (2 * PI * (z - gridSize / 2)) / gridSize;
    
    // Compute the magnitude of the wave vector
    float k = sqrt(kx * kx + kz * kz);
    
    if (k == 0.0f)
    {
        return;  // Skip for zero-frequency component (DC component)
    }

    float spectrum = PhillipsSpectrum(k);

    SpectrumRenderTarget[tid.xy] = float4(spectrum, 0.0f, 0.0f, 0.0f);
    
    // float real = spectrum * Random(seed) * 0.5f;
    // float imag = spectrum * Random(seed + 1) * 0.5f;

    float real, imag;
    GenerateGaussianRandom(tid, real, imag);
    real *= sqrt(spectrum) / sqrt(2.0f);
    imag *= sqrt(spectrum) / sqrt(2.0f);
    
    HeightmapRenderTarget[tid.xy] = float4(real, imag, 0.0f, 0.0f);
}