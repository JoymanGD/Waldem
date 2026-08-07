#include "TerrainCommon.hlsl"

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct PS_OUTPUT
{
    float4 WorldPositionRT : SV_TARGET0;
    float4 NormalRT : SV_TARGET1;
    float4 ColorRT : SV_TARGET2;
    float4 ORM : SV_TARGET3;
    int2 MeshIDRT : SV_TARGET4;
};

SamplerState myStaticSampler : register(s0);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    StructuredBuffer<TerrainSceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferId];
    TerrainSceneData sceneData = sceneDataBuffer[0];

    float4 color = float4(1, 1, 1, 1);
    
    if(sceneData.AlbedoIndex > 0)
    {
        Texture2D<float4> albedoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(sceneData.AlbedoIndex)];
        float4 albedo = albedoTexture.Sample(myStaticSampler, input.UV);
        color *= albedo;
    }
    
    float4 normal = float4(input.Normal, 0.0f);

    float4 orm = float4(0.0f, 1, 0, 0.0f);
    
    output.ColorRT = color;
    output.NormalRT = normalize(mul(sceneData.WorldTransform, normal));
    output.WorldPositionRT = input.WorldPosition;
    output.ORM = orm;
    output.MeshIDRT.r = 1;
    output.MeshIDRT.g = input.MeshId+1;
    return output;
}
