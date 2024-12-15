struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

StructuredBuffer<float4x4> WorldTransforms : register(t2);

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.Position = float4(input.Position, 1.0f);
    output.UV = input.UV;

    return output;
}