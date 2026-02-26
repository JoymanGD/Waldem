SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint MeshIDRTID;
    uint RadianceRTID;
    uint TargetRTID;
    uint SkyColorRTID;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    Texture2D<int2> meshIDsRT = ResourceDescriptorHeap[MeshIDRTID];
    Texture2D radianceRT = ResourceDescriptorHeap[RadianceRTID];
    Texture2D skyColorRT = ResourceDescriptorHeap[SkyColorRTID];
    RWTexture2D<float4> TargetRT = ResourceDescriptorHeap[TargetRTID];

    int2 meshTypeId = meshIDsRT.Load(int3(tid, 0));
    
    float4 skyColor = skyColorRT.Load(int3(tid,0));

    float2 screenSize;
    TargetRT.GetDimensions(screenSize.x, screenSize.y);

    float3 finalColor;
    if (meshTypeId.g > 0)
    {
        finalColor = radianceRT.Load(uint3(tid, 0));
    }
    else
    {
        finalColor = skyColor.rgb;
    }

    TargetRT[tid] = float4(finalColor, 1.0f);
}