#include "Core.hlsl"

struct Light
{
    float3 Position;
    uint Type;
    float3 Direction;
    float Intensity;
    float3 Color;
    float Range;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    uint MeshId : MESH_ID;
};

SamplerState myStaticSampler : register(s0);

StructuredBuffer<Light> Lights : register(t0);
Texture2D Shadowmap : register(t1);
Texture2D DiffuseTextures[MAX_TEXTURES] : register(t2);

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DiffuseTextures[input.MeshId].Sample(myStaticSampler, input.UV);
    float shadowmap = Shadowmap.Sample(myStaticSampler, input.UV).r;

    if(color.a < 0.1f)
        discard;
    
    Light light = Lights[0];

    float3 resultColor = color.rgb + light.Color * light.Intensity * saturate(dot(input.Normal, -light.Direction));
    resultColor *= shadowmap;
    
    return float4(resultColor, 1.0f);
}
