#include "Shading.hlsl"

SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint2 MousePos;
    uint AlbedoRTID;
    uint MeshIDRTID;
    uint RadianceRTID;
    uint TargetRTID;
    uint SkyColorRTID;
    uint DepthRTID;
    uint HoveredMeshesID;
    uint SceneDataID;
};

struct SceneData
{
    float4x4 InverseProjection;
    float4x4 InverseView;
    float4x4 ViewProjection;
    float4 SkyZenithColor;
    float4 SkyHorizonColor;
    float4 GroundColor;
    float4 SunDirection;
    float4 CameraPosition;
};

float GridLinesAA(uint2 tid, float2 screenSize, float3 rayOrigin, float4x4 InvProj, float4x4 InvView, float thickness)
{
    float2 uvCenter = (tid + 0.5f) / screenSize;
    uvCenter.y = 1.0f - uvCenter.y;

    float2 ndcC = uvCenter * 2.0f - 1.0f;
    float4 clipC = float4(ndcC, 1.0f, 1.0f);
    float4 viewC = mul(InvProj, clipC); viewC /= viewC.w;
    float3 dirC = normalize(mul(InvView, float4(viewC.xyz, 0.0f)).xyz);
    float tC = -rayOrigin.y / dirC.y;
    float2 p = (rayOrigin + dirC * tC).xz;

    float2 uvX = (tid + uint2(1,0) + 0.5f) / screenSize;
    uvX.y = 1.0f - uvX.y;
    float2 ndcX = uvX * 2.0f - 1.0f;
    float4 clipX = float4(ndcX, 1.0f, 1.0f);
    float4 viewX = mul(InvProj, clipX); viewX /= viewX.w;
    float3 dirX = normalize(mul(InvView, float4(viewX.xyz, 0.0f)).xyz);
    float tX = -rayOrigin.y / dirX.y;
    float2 pX = (rayOrigin + dirX * tX).xz;

    float2 uvY = (tid + uint2(0,1) + 0.5f) / screenSize;
    uvY.y = 1.0f - uvY.y;
    float2 ndcY = uvY * 2.0f - 1.0f;
    float4 clipY = float4(ndcY, 1.0f, 1.0f);
    float4 viewY = mul(InvProj, clipY); viewY /= viewY.w;
    float3 dirY = normalize(mul(InvView, float4(viewY.xyz, 0.0f)).xyz);
    float tY = -rayOrigin.y / dirY.y;
    float2 pY = (rayOrigin + dirY * tY).xz;

    float2 ddx = pX - p;
    float2 ddy = pY - p;

    float2 w = max(abs(ddx), abs(ddy)) + 1e-6;
    float2 d = 0.5 - abs(frac(p) - 0.5);

    float2 a = saturate((d - thickness * 0.5) / w);
    float2 b = saturate((d + thickness * 0.5) / w);

    float2 coverage = b - a;
    return max(coverage.x, coverage.y);
}

float3 GetWorldDir(float2 uv, float4x4 InverseProjection, float4x4 InverseView)
{
    float2 ndc = uv * 2.0f - 1.0f;
    float4 clip = float4(ndc, 1.0f, 1.0f);

    float4 view = mul(InverseProjection, clip);
    view /= view.w;

    float4 world = mul(InverseView, float4(view.xyz, 0.0f));
    return normalize(world);
}

float3 DrawGrid(uint2 tid, float2 screenSize, SceneData sceneData, Texture2D<float> depthRT, float3 finalColor)
{
    float2 uv = (tid + 0.5f) / screenSize;
    uv.y = 1.0f - uv.y;

    float3 worldDir = GetWorldDir(uv, sceneData.InverseProjection, sceneData.InverseView);
    float3 rayOrigin = sceneData.CameraPosition.xyz;

    float gridT = -rayOrigin.y / worldDir.y;
    if (gridT >= 0)
    {
        float3 worldPos = rayOrigin + worldDir * gridT;

        float lines = GridLinesAA(tid, screenSize, rayOrigin, sceneData.InverseProjection, sceneData.InverseView, 0.02f);

        float dist = length(worldPos.xz - rayOrigin.xz);
        float fade = saturate(1.0f - dist / 100.0f);
        fade *= saturate(1.0f - dist / 200.0f);
        lines *= fade;

        float sceneDepth = depthRT.Load(int3(tid, 0));
        float2 ndc = uv * 2.0f - 1.0f;
        float4 clip = float4(ndc, sceneDepth, 1.0f);

        float4 viewPos = mul(sceneData.InverseProjection, clip);
        viewPos /= viewPos.w;
        float3 scenePos = mul(sceneData.InverseView, float4(viewPos.xyz, 1.0f)).xyz;

        float sceneT = length(scenePos - rayOrigin);

        if (gridT < sceneT - 0.01f)
        {
            float3 lineColor = float3(1.0, 1.0, 1.0);
            return lerp(finalColor, lineColor, lines);
        }
    }

    return finalColor;
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> hoveredMeshes = ResourceDescriptorHeap[HoveredMeshesID];
    Texture2D albedoRT    = ResourceDescriptorHeap[AlbedoRTID];
    Texture2D meshIDsRT   = ResourceDescriptorHeap[MeshIDRTID];
    Texture2D radianceRT  = ResourceDescriptorHeap[RadianceRTID];
    Texture2D skyColorRT  = ResourceDescriptorHeap[SkyColorRTID];
    Texture2D<float> depthRT = ResourceDescriptorHeap[DepthRTID];
    RWTexture2D<float4> TargetRT = ResourceDescriptorHeap[TargetRTID];
    StructuredBuffer<SceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataID];
    SceneData sceneData = sceneDataBuffer[0];

    int meshId = asint(meshIDsRT.Load(int3(tid, 0)).x);
    hoveredMeshes[0] = asint(meshIDsRT.Load(int3(MousePos, 0)).x);

    float4 skyColor = skyColorRT.Load(int3(tid,0));

    float2 screenSize;
    TargetRT.GetDimensions(screenSize.x, screenSize.y);

    float3 finalColor;
    if (meshId > 0)
    {
        finalColor = radianceRT.Load(uint3(tid, 0));
    }
    else
    {
        finalColor = skyColor.rgb;
    }

    finalColor = DrawGrid(tid, screenSize, sceneData, depthRT, finalColor);

    TargetRT[tid] = float4(finalColor, 1.0f);
}
