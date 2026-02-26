#include "Materials.hlsl"

cbuffer RootConstants : register(b0)
{
    uint WorldTransformsBufferID;
    uint MaterialAttributesBufferID;
    uint SceneDataBufferID;
    uint MeshIDRTID;
};
cbuffer IndirectRootConstants : register(b1)
{
    uint PackedId;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    uint GizmoType : GIZMO_TYPE;
    uint MeshId : MESH_ID;
};

struct PS_OUTPUT
{
    float4 ColorRT : SV_TARGET0;
    int2 MeshIDRT : SV_TARGET1;
};

SamplerState myStaticSampler : register(s0);

float4 main(PSInput pin) : SV_Target
{
    RWTexture2D<int2> meshIDRTTex = ResourceDescriptorHeap[NonUniformResourceIndex(MeshIDRTID)];
    uint2 texCoords = uint2(pin.Position.xy);
    meshIDRTTex[texCoords] = int2(6, pin.MeshId + 1);
    
    StructuredBuffer<MaterialAttribute> matBuffer = ResourceDescriptorHeap[MaterialAttributesBufferID];
    MaterialAttribute matAttr = matBuffer[pin.GizmoType];

    float4 color = pin.Color * matAttr.Albedo;

    if(matAttr.DiffuseTextureIndex != -1)
    {
        Texture2D<float4> tex = ResourceDescriptorHeap[NonUniformResourceIndex(matAttr.DiffuseTextureIndex)];
        float4 texColor = tex.Sample(myStaticSampler, pin.UV);

        // Instead of discard, fade out color
        if(texColor.a < 0.1f)
        {
            discard;
        }
        
        color *= texColor;
    }

    return color;
}