struct ParticleSystem
{
    uint ParticlesAmount;
    float4 Color;
    float2 SizeMinMax;
    float2 SpeedMinMax;
    float2 LifetimeMinMax;
    int Shape;
    float Radius;
};

struct Particle
{
    float4 Color;
    float3 Position;
    float Lifetime;
    float3 Velocity;
    float Age;
    float3 Size;
    float Padding3;
};

cbuffer RootConstants : register(b0)
{
    uint WorldTransformsBufferID;
    uint SceneDataBufferID;
    float Time;
    float DeltaTime;
    uint ParticleSystemBufferId;
    uint ParticleBuffersIndicesBufferID;
    uint ParticlesBufferId;
};