#include "Shading.hlsl"

SamplerState myStaticSampler : register(s0);
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D WorldPositionRT : register(t2);
Texture2D NormalRT : register(t3);
Texture2D ColorRT : register(t4);
Texture2D ORMRT : register(t5);
Texture2D MeshIDRT : register(t6);
Texture2D DepthRT : register(t7);
RWTexture2D<float4> TargetRT : register(u0);
RWStructuredBuffer<int> HoveredMeshes : register(u1);

cbuffer MyConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
    matrix invView;
    matrix invProj;
};

cbuffer RootConstants : register(b1)
{
    float2 MousePosition;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float4 worldPosition = WorldPositionRT.Load(int3(tid, 0));
    float3 normal = NormalRT.Load(int3(tid, 0)).xyz;
    float4 albedo = ColorRT.Load(int3(tid, 0));
    float4 orm = ORMRT.Load(int3(tid, 0));

    float depthMousePos = DepthRT.Load(int3(MousePosition, 0)).x;
    if(depthMousePos > 0.999f) //sky
    {
        HoveredMeshes[0] = -1;
    }
    else
    {
        HoveredMeshes[0] = asint(MeshIDRT.Load(int3(MousePosition, 0)).x);
    }

    float3 resultColor = GetResultColor(Lights[0], Shadowmap, cmpSampler, worldPosition, normal, albedo, orm, invView);
    
    //Writing the result to the render target
    TargetRT[tid] = float4(resultColor, 1.0f);
}