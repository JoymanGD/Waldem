#define INTERLOCK_SCALE_VALUE 10000000.0f
SamplerState myStaticSampler : register(s0);

Texture2D WorldPositionRT : register(t0);
Texture2D DepthRT : register(t1);
RWStructuredBuffer<float3> AverageWorldPositions : register(u0);

groupshared int sharedAveragePositionX;
groupshared int sharedAveragePositionY;
groupshared int sharedAveragePositionZ;
groupshared uint sharedCount;

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    if (tid.x == 0 && tid.y == 0)
    {
        sharedAveragePositionX = 0;
        sharedAveragePositionY = 0;
        sharedAveragePositionZ = 0;
        sharedCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    float depth = DepthRT.Load(int3(tid, 0)).r;
    float4 worldPos = WorldPositionRT.Load(int3(tid, 0));

    if(depth <= 0.99999f)
    {
        InterlockedAdd(sharedAveragePositionX, int(worldPos.x));
        InterlockedAdd(sharedAveragePositionY, int(worldPos.y));
        InterlockedAdd(sharedAveragePositionZ, int(worldPos.z));
        InterlockedAdd(sharedCount, 1);
    }
    
    GroupMemoryBarrierWithGroupSync();

    if (tid.x == 0 && tid.y == 0)
    {
        if (sharedCount > 0)
        {
            float3 summedPosition = float3(
                float(sharedAveragePositionX),
                float(sharedAveragePositionY),
                float(sharedAveragePositionZ)
            );
            summedPosition /= float(sharedCount);
            AverageWorldPositions[0] = summedPosition;
        }
    }
}