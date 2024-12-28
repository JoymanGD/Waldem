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

// Generate a random float in the range [0, 1]
float Random(uint seed)
{
    uint randValue = xorshift32(seed);
    return (randValue & 0xFFFFFFF) / float(0xFFFFFFF); // Normalize to [0, 1)
}

void GenerateGaussianRandoms(uint seed, out float real, out float imag)
{
    // Generate two uniform random values
    float u1 = Random(seed);
    float u2 = Random(seed + 1);

    // Apply the Box-Muller transform
    float r = sqrt(-2.0f * log(u1));
    float theta = 2.0f * 3.14159265359f * u2;

    // Gaussian-distributed random numbers
    real = r * cos(theta);
    imag = r * sin(theta);
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
    float k = sqrt(kx * kx + kz * kz);
    
    if (k == 0.0f)
    {
        return;  // Skip for zero-frequency component (DC component)
    }

    float spectrum = PhillipsSpectrum(k);

    SpectrumRenderTarget[tid] = float4(spectrum, 0.0f, 0.0f, 0.0f);
    
    uint seed = x + z * gridSize; // Using x and z as seed for randomness

    // float real = spectrum * Random(seed) * 0.5f;
    // float imag = spectrum * Random(seed + 1) * 0.5f;

    float real, imag;
    GenerateGaussianRandoms(seed, real, imag);
    // real *= sqrt(spectrum) / sqrt(2.0f);
    // imag *= sqrt(spectrum) / sqrt(2.0f);
    
    HeightmapRenderTarget[tid] = float4(real, imag, 0.0f, 0.0f);
}