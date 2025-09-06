cbuffer RootConstants : register(b0)
{
    uint WorldTransformsBufferID;
    uint SceneDataBufferID;
    uint ParticlesBufferID;
    uint ParticleSystemDataBufferID;
    float DeltaTime;
};

cbuffer IndirectRootConstants : register(b1)
{
    uint MeshId;
};

struct SceneData
{
    float4x4 View;
    float4x4 Projection;
};

struct Particle
{
    float4 Color;
    float3 BasePosition;
    float Padding1;
    float3 Position;
    float Lifetime;
    float3 Velocity;
    float Age;
};

struct VSInput
{
    float3 Position : POSITION; // quad corner (-0.5,-0.5 .. 0.5,0.5)
    float4 Color    : COLOR;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float3 Bitangent: BITANGENT;
    float2 UV       : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
    float  Alpha    : TEXCOORD1;
    float4 Color    : COLOR;
};

VSOutput main(VSInput vin, uint instanceID : SV_InstanceID)
{
    VSOutput vout;

    StructuredBuffer<Particle> particles       = ResourceDescriptorHeap[ParticlesBufferID];
    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[WorldTransformsBufferID];
    StructuredBuffer<SceneData> sceneDataBuffer= ResourceDescriptorHeap[SceneDataBufferID];
    SceneData sceneData = sceneDataBuffer[0];

    Particle p = particles[instanceID];
    float4x4 worldTransform = worldTransforms[MeshId];

    // lifetime fade
    float lifeRatio = saturate(1.0f - p.Age / max(0.001f, p.Lifetime));

    float4x4 worldViewMatrix = mul(sceneData.View, worldTransform);
    float3 positionVS = vin.Position + float3(worldViewMatrix._14, worldViewMatrix._24, worldViewMatrix._34);
    float3 particlePosition = p.Position;
    float3 deltaViewPosition = mul(sceneData.View, float4(particlePosition, 0.0f)).xyz;
    positionVS += deltaViewPosition;
    vout.Position = mul(sceneData.Projection, float4(positionVS, 1.0f));

    vout.UV    = vin.UV;
    vout.Alpha = lifeRatio;
    vout.Color = p.Color * vin.Color;
    return vout;
}

