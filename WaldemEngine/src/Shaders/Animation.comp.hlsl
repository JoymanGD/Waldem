#include "Types.hlsl"

cbuffer RootConstants : register(b0)
{
    uint OriginalVertexBuffer;
    uint VertexBuffer;
    uint VertexOffset;
    float4x4 WorldTransform;
};

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    StructuredBuffer<Vertex> originalVertexBuffer = ResourceDescriptorHeap[OriginalVertexBuffer];
    RWStructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[VertexBuffer];

    Vertex vertex = originalVertexBuffer[tid.x];
    vertex.Position -= float4(0, 1, 0, 0);

    vertexBuffer[VertexOffset + tid.x] = vertex;
}