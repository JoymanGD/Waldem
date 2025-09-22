SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint2 MousePos;
    uint MeshIDRTID;
    uint RadianceRTID;
    uint TargetRTID;
    uint SkyColorRTID;
    uint HoveredMeshesID;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> hoveredMeshes = ResourceDescriptorHeap[HoveredMeshesID];
    Texture2D meshIDsRT   = ResourceDescriptorHeap[MeshIDRTID];
    Texture2D radianceRT  = ResourceDescriptorHeap[RadianceRTID];
    Texture2D skyColorRT  = ResourceDescriptorHeap[SkyColorRTID];
    RWTexture2D<float4> TargetRT = ResourceDescriptorHeap[TargetRTID];

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

    TargetRT[tid] = float4(finalColor, 1.0f);
}