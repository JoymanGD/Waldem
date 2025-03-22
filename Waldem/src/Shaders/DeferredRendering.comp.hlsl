#include "Shading.hlsl"
#include "Shadows.hlsl"
#include "Lighting.hlsl"

SamplerState myStaticSampler : register(s0);
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<LightTransformData> LightTransformDatas : register(t0);
Texture2D WorldPositionRT : register(t1);
Texture2D NormalRT : register(t2);
Texture2D ColorRT : register(t3);
Texture2D ORMRT : register(t4);
Texture2D MeshIDRT : register(t5);
Texture2D DepthRT : register(t6);
Texture2D RadianceRT : register(t7);
RWTexture2D<float4> TargetRT : register(u0);
RWStructuredBuffer<int> HoveredMeshes : register(u1);

cbuffer MyConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
    matrix invView;
    matrix invProj;
    int NumLights;
};

cbuffer RootConstants : register(b1)
{
    float2 MousePosition;
};

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float4 worldPosition = WorldPositionRT.Load(int3(tid, 0));
    float3 normal = NormalRT.Load(int3(tid, 0)).xyz;
    float4 albedo = ColorRT.Load(int3(tid, 0));
    float4 orm = ORMRT.Load(int3(tid, 0));

    HoveredMeshes[0] = asint(MeshIDRT.Load(int3(MousePosition, 0)).x);

    float3 radiance = RadianceRT.Load(uint3(tid, 0));
    float3 ambient = albedo * AMBIENT;
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < NumLights; i++)
    {
        uint lightType = LightTransformDatas[i].Type;
        float3 lightDir = float3(0.0f, 0.0f, 0.0f);
        
        if(lightType == 0) //Directional
        {
           lightDir  = -normalize(LightTransformDatas[i].Forward);
        }
        else if(lightType == 1) //Point
        {
            lightDir = normalize(LightTransformDatas[i].Position - worldPosition.xyz);
        }
        else if(lightType == 2) //Spot
        {
            lightDir = LightTransformDatas[i].Position - worldPosition.xyz;
        }
        
        diffuse += GetDiffuseColor(lightDir, worldPosition, normal, albedo, orm, invView);
    }
    
    //Writing the result to the render target
    TargetRT[tid] = float4(ambient + diffuse * radiance, 1.0f);
}