#include "Shading.hlsl"

SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint2 MousePos;
    uint AlbedoRTID;
    uint MeshIDRTID;
    uint RadianceRTID;
    uint TargetRTID;
    uint HoveredMeshesID;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> hoveredMeshes = ResourceDescriptorHeap[HoveredMeshesID];
    Texture2D albedoRT = ResourceDescriptorHeap[AlbedoRTID];
    Texture2D meshIDsRT = ResourceDescriptorHeap[MeshIDRTID];
    Texture2D radianceRT = ResourceDescriptorHeap[RadianceRTID];
    RWTexture2D<float4> TargetRT = ResourceDescriptorHeap[TargetRTID];
    hoveredMeshes[0] = asint(meshIDsRT.Load(int3(MousePos, 0)).x);
    
    float4 albedo = albedoRT.Load(int3(tid, 0));
    float3 radiance = radianceRT.Load(uint3(tid, 0));
    float3 ambient = albedo * AMBIENT;
    TargetRT[tid] = float4(ambient + radiance, 1.0f);
}