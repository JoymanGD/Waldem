#include "PhysicsTypes.hlsl"

StructuredBuffer<AABB> AABBs : register(t0);
RWStructuredBuffer<BVHNode> BVHNodes : register(u0);

AABB MergeBoundingBoxes(AABB a, AABB b)
{
    AABB merged;
    merged.min = min(a.min, b.min);
    merged.max = max(a.max, b.max);
    return merged;
}

cbuffer CollisionConstantsCB : register(b0)
{
    uint ComponentCount;
    uint MaxObjectsPerThread;
}

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
}
