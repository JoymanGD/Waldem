#include "Color.hlsl"
#include "Curve.hlsl"

struct ParticleSystem
{
    uint ParticlesAmount;
    int Shape;
    float Radius;
    uint WorldSpace;
    float4 Color;
    Gradient ColorOverLifetime;
    float2 SizeMinMax;
    float2 SpeedMinMax;
    float2 LifetimeMinMax;
    Curve SizeOverLifetime;
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
    uint SpawnId;
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