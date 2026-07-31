cbuffer RootConstants : register(b0)
{
    uint InstanceBufferID;
    uint SceneDataBufferID;
};

struct GridInstanceData
{
    float4x4 WorldMatrix;
    float4 Color;
};

struct SceneData
{
    float4x4 ViewProjection;
};

struct VSInput
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    StructuredBuffer<GridInstanceData> instanceBuffer = ResourceDescriptorHeap[InstanceBufferID];
    StructuredBuffer<SceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];

    GridInstanceData instanceData = instanceBuffer[instanceID];
    float4 worldPosition = mul(instanceData.WorldMatrix, input.Position);

    VSOutput output;
    output.Position = mul(sceneDataBuffer[0].ViewProjection, worldPosition);
    output.Color = instanceData.Color;
    return output;
}
