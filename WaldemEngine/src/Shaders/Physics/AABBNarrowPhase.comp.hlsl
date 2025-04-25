#include "PhysicsTypes.hlsl"

RWStructuredBuffer<PhysicsComponent> PhysicsComponents : register(u0);
StructuredBuffer<float4x4> Transforms : register(t0);

cbuffer CollisionConstantsCB : register(b0)
{
    int ComponentCount;
}

[numthreads(8, 1, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    if(tid.x >= ComponentCount)
    {
        return;
    }

    PhysicsComponents[tid.x].Linear.Velocity = 5;
}