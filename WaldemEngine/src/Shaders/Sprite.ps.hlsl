cbuffer RootConstants : register(b0)
{
    uint WorldTransforms;
    uint SpriteIds;
    uint SceneDataBufferId;
};

cbuffer IndirectRootConstants : register(b1)
{
    uint MeshId;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
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
    int MeshIDRT : SV_TARGET4;
};

SamplerState myStaticSampler : register(s0);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[WorldTransforms];
    StructuredBuffer<int> spriteIds = ResourceDescriptorHeap[SpriteIds];

    float4 color = input.Color;

    Texture2D<float4> ColorTexture = ResourceDescriptorHeap[spriteIds[input.MeshId]];
    float4 sampledColor = ColorTexture.Sample(myStaticSampler, input.UV);
    
    if(sampledColor.a < 0.1f)
        discard;
    
    color *= sampledColor;
    
    output.ColorRT = color;
    output.NormalRT = normalize(mul(worldTransforms[MeshId], input.Normal));
    output.WorldPositionRT = input.WorldPosition;
    output.ORM = float4(0.0f, 1, 0, 0.0f);
    output.MeshIDRT = MeshId+1;
    return output;
}
