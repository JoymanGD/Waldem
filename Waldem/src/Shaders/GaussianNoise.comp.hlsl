#define PI 3.14159265359f

RWTexture2D<float4> GaussianNoiseRenderTarget : register(u0);

cbuffer MyPushConstants : register(b0)
{
    float ElapsedTime;
}

// MT19937 constants
#define MT19937_N 624
#define MT19937_M 397
#define MT19937_MATRIX_A 0x9908b0df
#define MT19937_UPPER_MASK 0x80000000
#define MT19937_LOWER_MASK 0x7fffffff

// Function to initialize the state array
void mt19937_seed(uint seed, out uint mtState[MT19937_N], out uint mtIndex)
{
    mtState[0] = seed;
    for (uint i = 1; i < MT19937_N; ++i)
    {
        mtState[i] = 0x6c078965 * (mtState[i - 1] ^ (mtState[i - 1] >> 30)) + i;
    }
    mtIndex = MT19937_N; // The state is now ready
}

// Function to generate a random number from the Mersenne Twister state
uint mt19937_generate(inout uint mtState[MT19937_N], inout uint mtIndex)
{
    if (mtIndex >= MT19937_N) 
    {
        uint i;
        for (i = 0; i < MT19937_N - MT19937_M; ++i)
        {
            uint y = (mtState[i] & MT19937_UPPER_MASK) | (mtState[i + 1] & MT19937_LOWER_MASK);
            mtState[i] = mtState[i + MT19937_M] ^ (y >> 1) ^ ((y & 1u) ? MT19937_MATRIX_A : 0u);
        }
        for (; i < MT19937_N - 1; ++i)
        {
            uint y = (mtState[i] & MT19937_UPPER_MASK) | (mtState[i + 1] & MT19937_LOWER_MASK);
            mtState[i] = mtState[i + (MT19937_M - MT19937_N)] ^ (y >> 1) ^ ((y & 1u) ? MT19937_MATRIX_A : 0u);
        }
        uint y = (mtState[MT19937_N - 1] & MT19937_UPPER_MASK) | (mtState[0] & MT19937_LOWER_MASK);
        mtState[MT19937_N - 1] = mtState[MT19937_M - 1] ^ (y >> 1) ^ ((y & 1u) ? MT19937_MATRIX_A : 0u);

        mtIndex = 0;
    }

    uint y = mtState[mtIndex++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= y >> 18;

    return y;
}

// Function to generate a float in the range [0, 1)
float mt19937_random_float(inout uint mtState[MT19937_N], inout uint mtIndex)
{
    return (float)(mt19937_generate(mtState, mtIndex) >> 8) / (float)(1u << 24);
}

// Box-Muller transform to generate Gaussian random numbers
void GenerateGaussianRandom(uint2 dispatchThreadID, out float real, out float imag)
{
	uint mtState[MT19937_N];
    uint mtIndex;
    uint timeInt = asuint(ElapsedTime);

    // Initialize the Mersenne Twister state using a unique seed based on dispatchThreadID
    uint seed = dispatchThreadID.x + dispatchThreadID.y * 12345 + timeInt;
    mt19937_seed(seed, mtState, mtIndex);

    // Generate two uniform random numbers in the range [0, 1)
    float u1 = mt19937_random_float(mtState, mtIndex);
    float u2 = mt19937_random_float(mtState, mtIndex);

    u1 = max(u1, 1e-7);

    // Apply the Box-Muller transform
    float z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * PI * u2);
    float z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * PI * u2);

    // Return the Gaussian-distributed random numbers
    real = z0;
    imag = z1;
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float real, imag;
    GenerateGaussianRandom(tid, real, imag);
    
    GaussianNoiseRenderTarget[tid] = float4(real, imag, 0.0f, 0.0f); 
}