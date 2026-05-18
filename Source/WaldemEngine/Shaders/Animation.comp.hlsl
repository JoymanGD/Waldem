#include "Types.hlsl"

struct VertexBones
{
    int4   Joints;
    float4 Weights;
};

cbuffer RootConstants : register(b0)
{
    uint BindPoseVertexBuffer;
    uint SkinnedVertexBuffer;
    uint VertexBonesBuffer;
    uint BoneMatricesBuffer;
    uint VertexOffset;
    uint VertexCount;
};

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= VertexCount)
        return;

    StructuredBuffer<Vertex>     bindPoseVB   = ResourceDescriptorHeap[BindPoseVertexBuffer];
    RWStructuredBuffer<Vertex>   skinnedVB    = ResourceDescriptorHeap[SkinnedVertexBuffer];
    StructuredBuffer<VertexBones> bonesBuffer = ResourceDescriptorHeap[VertexBonesBuffer];
    StructuredBuffer<float4x4>   boneMatrices = ResourceDescriptorHeap[BoneMatricesBuffer];

    Vertex      v     = bindPoseVB[tid.x];
    VertexBones bones = bonesBuffer[tid.x];

    float4 skinnedPos       = float4(0, 0, 0, 0);
    float4 skinnedNormal    = float4(0, 0, 0, 0);
    float4 skinnedTangent   = float4(0, 0, 0, 0);
    float4 skinnedBitangent = float4(0, 0, 0, 0);

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (bones.Joints[i] < 0)
            continue;

        float    w = bones.Weights[i];
        float4x4 m = boneMatrices[bones.Joints[i]];

        skinnedPos       += w * mul(m, v.Position);
        skinnedNormal    += w * mul(m, v.Normal);
        skinnedTangent   += w * mul(m, v.Tangent);
        skinnedBitangent += w * mul(m, v.Bitangent);
    }

    Vertex output      = v;
    output.Position    = float4(skinnedPos.xyz, 1.0f);
    output.Normal      = float4(normalize(skinnedNormal.xyz), 0.0f);
    output.Tangent     = float4(normalize(skinnedTangent.xyz), 0.0f);
    output.Bitangent   = float4(normalize(skinnedBitangent.xyz), 0.0f);

    skinnedVB[VertexOffset + tid.x] = output;
}
