cbuffer RootConstants : register(b0)
{
    uint WorldTransformsBufferID;
    uint SceneDataBufferID;
    uint ParticlesBufferID;
    uint ParticleSystemDataBufferID;
    float DeltaTime;
};

struct ParticleSystemData
{
    float4 Color;
    float3 Size;
    uint ParticlesCount;
    float3 Acceleration;
    float Lifetime;
};

struct Particle
{
    float4 Color;
    float3 BasePosition;
    float Padding1;
    float3 Position;
    float  Lifetime;
    float3 Velocity;
    float  Age;
};

float rand(uint seed)
{
    seed = (seed << 13U) ^ seed;
    return (1.0f - ((seed * (seed * seed * 15731U + 789221U) + 1376312589U) & 0x7fffffff) / 1073741824.0f);
}

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    RWStructuredBuffer<Particle> particles = ResourceDescriptorHeap[ParticlesBufferID];
    StructuredBuffer<ParticleSystemData> particleSystemDataBuffer = ResourceDescriptorHeap[ParticleSystemDataBufferID];
    ParticleSystemData psData = particleSystemDataBuffer[0];

    uint id = tid.x;
    if (id >= psData.ParticlesCount) return;

    Particle p = particles[id];

    p.Age += DeltaTime;
    if (p.Age >= p.Lifetime)
    {
        p.BasePosition = float3(0, 0, 0);

        p.Velocity = float3(
            (rand(id * 13) - 0.5f) * 2.0f,
            abs(rand(id * 97)) * 2.0f,
            (rand(id * 31) - 0.5f) * 2.0f
        );
        p.Lifetime = psData.Lifetime * (0.5f + rand(id * 7));
        p.Age = 0.0f;
    }
    else
    {
        p.Velocity += psData.Acceleration * DeltaTime;
        p.BasePosition += p.Velocity * DeltaTime;
    }

    p.Position = p.BasePosition;

    p.Color = psData.Color;

    particles[id] = p;
}
