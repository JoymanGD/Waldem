#include "Shading.hlsl"

SamplerState myStaticSampler : register(s0);

Texture2D AlbedoRT : register(t0);
Texture2D MeshIDRT : register(t1);
Texture2D RadianceRT : register(t2);
RWTexture2D<float4> TargetRT : register(u0);
RWStructuredBuffer<int> HoveredMeshes : register(u1);

cbuffer RootConstants : register(b0)
{
    int2 MousePosition;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    HoveredMeshes[0] = asint(MeshIDRT.Load(int3(MousePosition, 0)).x);
    
    float4 albedo = AlbedoRT.Load(int3(tid, 0));
    float3 radiance = RadianceRT.Load(uint3(tid, 0));
    float3 ambient = albedo * AMBIENT;
    TargetRT[tid] = float4(ambient + radiance, 1.0f);
}