#include "ParticleSystem.hlsl"
#include "Utils/Random.hlsl"

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    RWStructuredBuffer<Particle> particles = ResourceDescriptorHeap[ParticlesBufferId];
    RWStructuredBuffer<ParticleSystem> particleSystemBuffer = ResourceDescriptorHeap[ParticleSystemBufferId];
    ParticleSystem particleSystem = particleSystemBuffer[0];

    uint id = tid.x;
    
    if (id >= particleSystem.ParticlesAmount) return;
    
    Particle p = particles[id];

    p.WorldSpace = particleSystem.WorldSpace;

    p.Age += DeltaTime;
    if (p.Age >= p.Lifetime)
    {
        float2 randomInCircle = RandomInCircle(Time, id * 3.65438f, particleSystem.Radius);
        p.Position = p.WorldSpace ? mul(WorldTransform, float4(randomInCircle.x, 0, randomInCircle.y, 1)) : float3(randomInCircle.x, 0, randomInCircle.y);

        p.Velocity = 0;
        p.Lifetime = RandomRange(Time, id * 7.38f, particleSystem.LifetimeMinMax.x, particleSystem.LifetimeMinMax.y);
        p.Age = 0.0f;
        float randomSize = RandomRange(Time, id * 4.35558f, particleSystem.SizeMinMax.x, particleSystem.SizeMinMax.y);

        float3 scale = float3(length(WorldTransform[0].xyz), length(WorldTransform[1].xyz), length(WorldTransform[2].xyz));
        p.Size = mul(scale, float3(randomSize, randomSize, randomSize));
    }
    else
    {
        float randomSpeed = RandomRange(Time, id * 12.382315f, particleSystem.SpeedMinMax.x, particleSystem.SpeedMinMax.y);
        // p.Velocity += mul(WorldTransform, float4(0, 1, 0, 0)) * randomSpeed * DeltaTime;
        p.Velocity += (p.WorldSpace ? mul(WorldTransform, float4(0, 1, 0, 0)) : float3(0, 1, 0)) * randomSpeed * DeltaTime;
        p.Position += p.Velocity * DeltaTime, 1.0f;
    }

    p.Color = particleSystem.Color;

    particles[id] = p;
}
