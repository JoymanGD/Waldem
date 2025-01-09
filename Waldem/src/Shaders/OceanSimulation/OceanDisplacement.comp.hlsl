#ifdef _DXC_COMPILER
    #include "Types.hlsl"
#else
    #include "../Types.hlsl"
#endif

SamplerState myStaticSampler : register(s0);

Texture2D Normal : register(t0);
Texture2D Displacement : register(t1);
RWStructuredBuffer<Vertex> VertexBuffer : register(u0);

[numthreads(16, 16, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float3 displacement = Displacement[tid].rgb;
    float3 normal = Normal[tid].rgb * 2.0f - 1.0f;
    
    uint index = tid.y * 512 + tid.x;

    Vertex vertex = VertexBuffer[index];

    vertex.Position.x += displacement.x * 0.02f;
    vertex.Position.y = displacement.y * 1.5f;
    vertex.Position.z += displacement.z * 0.02f;

    vertex.Normal = normal;

    VertexBuffer[index] = vertex;
}