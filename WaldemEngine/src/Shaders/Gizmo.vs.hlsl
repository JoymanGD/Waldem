cbuffer RootConstants : register(b0)
{
    uint WorldTransformsBufferID;
    uint MaterialAttributesBufferID;
    uint SceneDataBufferID;
    uint MeshIDRTID;
};
cbuffer IndirectRootConstants : register(b1)
{
    uint PackedId; // [31:16] = transform index, [15:0] = gizmo type
};

struct SceneData
{
    float4x4 View;
    float4x4 Projection;
};

struct VSInput
{
    float3 Position : POSITION;  // quad corner (-0.5..0.5)
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    uint GizmoType : GIZMO_TYPE;
    uint MeshId : MESH_ID;
};

VSOutput main(VSInput vin)
{
    VSOutput vout;

    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[WorldTransformsBufferID];
    StructuredBuffer<SceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
    SceneData sceneData = sceneDataBuffer[0];

    uint meshId = PackedId >> 16;
    uint gizmoType = PackedId & 0xFFFF;

    float4x4 world = worldTransforms[meshId];
    float4x4 worldView = mul(sceneData.View, world);

    float3 centerVS = float3(worldView._14, worldView._24, worldView._34);

    float screenSize = 0.1f; // ~5% of screen height

    float scale = screenSize * centerVS.z;

    float3 positionVS = centerVS + vin.Position * scale;

    vout.Position = mul(sceneData.Projection, float4(positionVS, 1.0f));

    vout.Color = vin.Color;
    vout.UV = vin.UV;
    vout.GizmoType = gizmoType;
    vout.MeshId = meshId;
    return vout;
}
