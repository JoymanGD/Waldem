SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint2 MousePos;
    uint MeshIDRTID;
    uint HoveredMeshesID;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    RWStructuredBuffer<int2> hoveredMeshes = ResourceDescriptorHeap[HoveredMeshesID];
    Texture2D<int2> meshIDsRT = ResourceDescriptorHeap[MeshIDRTID];

    int2 mouseMeshTypeId = meshIDsRT.Load(int3(MousePos, 0));
    hoveredMeshes[0] = int2(mouseMeshTypeId.r, mouseMeshTypeId.g);
}