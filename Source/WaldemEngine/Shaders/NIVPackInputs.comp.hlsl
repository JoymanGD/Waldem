#pragma shader_model 6_6

cbuffer RootConstants : register(b0)
{
    uint WorldPositionRTID;
    uint WorldNormalRTID;
    uint MeshIDRTID;
    uint OutWorldPositionBufferID;
    uint OutWorldNormalValidBufferID;
    uint Width;
    uint Height;
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    if (tid.x >= Width || tid.y >= Height)
    {
        return;
    }

    Texture2D<float4> worldPosRT = ResourceDescriptorHeap[WorldPositionRTID];
    Texture2D<float4> worldNormalRT = ResourceDescriptorHeap[WorldNormalRTID];
    Texture2D<int2> meshIDsRT = ResourceDescriptorHeap[MeshIDRTID];
    RWStructuredBuffer<float4> outWorldPosition = ResourceDescriptorHeap[OutWorldPositionBufferID];
    RWStructuredBuffer<float4> outWorldNormalValid = ResourceDescriptorHeap[OutWorldNormalValidBufferID];

    const uint linearIndex = tid.y * Width + tid.x;

    const float4 pos = worldPosRT.Load(int3(tid, 0));
    const float4 nrm = worldNormalRT.Load(int3(tid, 0));
    const int2 meshTypeId = meshIDsRT.Load(int3(tid, 0));

    const float valid = meshTypeId.g > 0 ? 1.0f : 0.0f;

    outWorldPosition[linearIndex] = float4(pos.xyz, 1.0f);
    outWorldNormalValid[linearIndex] = float4(nrm.xyz, valid);
}
