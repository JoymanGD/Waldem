#include "OceanResources.hlsl"
#include "../Types.hlsl"

Texture2D Normal : register(t0);
Texture2D Displacement : register(t1);
StructuredBuffer<Vertex> VertexBufferOriginal : register(t2);
RWStructuredBuffer<Vertex> VertexBuffer : register(u0);

[numthreads(16, 16, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float2 UV = float2(tid) / float2(N, N);
    float motion = 0.00002f;
    float2 MovedUV = UV + normalize(W) * motion;
    uint2 MovedTid = uint2(MovedUV * float2(N, N));
    float3 displacement = Displacement[MovedTid].rgb;
    float3 normal = Normal[MovedTid].rgb * 2.0f - 1.0f;
    
    uint index = tid.y * N + tid.x;

    Vertex vertex = VertexBuffer[index];
    Vertex originalVertex = VertexBufferOriginal[index];

    vertex.Position.y = originalVertex.Position.y + displacement.y * WaveHeight;
    vertex.Position.x = originalVertex.Position.x - displacement.x * WaveChoppiness;
    vertex.Position.z = originalVertex.Position.z - displacement.z * WaveChoppiness;

    vertex.Normal = float3(normal.x, normal.z, normal.y);

    VertexBuffer[index] = vertex;
}