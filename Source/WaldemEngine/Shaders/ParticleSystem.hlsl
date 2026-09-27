struct ParticleSystem
{
    uint ParticlesAmount;
    float4 Color;
    float2 SizeMinMax;
    float2 SpeedMinMax;
    float2 LifetimeMinMax;
    int Shape;
    float Radius;
    uint WorldSpace;
};

struct Particle
{
    float4 Color;
    float3 Position;
    float Lifetime;
    float3 Velocity;
    float Age;
    float3 Size;
    uint WorldSpace;
};

cbuffer RootConstants : register(b0)
{
    float4x4 WorldTransform;
    uint WorldTransformsBufferID;
    uint SceneDataBufferID;
    float Time;
    float DeltaTime;
    uint ParticleSystemBufferId;
    uint ParticleBuffersIndicesBufferID;
    uint ParticlesBufferId;
};