#include "Shadows.hlsl"
#include "Lighting.hlsl"
#include "PBR.hlsl"

SamplerState myStaticSampler : register(s0);
SamplerComparisonState cmpSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);
Texture2D<float> Shadowmap : register(t1);
Texture2D WorldPositionRT : register(t2);
Texture2D NormalRT : register(t3);
Texture2D ColorRT : register(t4);
Texture2D MetalRoughnessRT : register(t5);
Texture2D MeshIDRT : register(t6);
Texture2D DepthRT : register(t7);
RWTexture2D<float4> TargetRT : register(u0);
RWStructuredBuffer<int> HoveredMeshes : register(u1);

cbuffer MyConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
};

cbuffer RootConstants : register(b1)
{
    float2 MousePosition;
};

float3 GetCameraToPixelDirection(float4x4 viewMatrix, float3 pixelWorldPosition)
{
    // Extract camera position from the view matrix
    // The inverse of the view matrix's translation part gives us the camera's world position
    float3 cameraPosition = -float3(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
    
    // Compute the direction from camera to pixel
    float3 direction = normalize(pixelWorldPosition - cameraPosition);
    
    return direction;
}

float3 GetForwardVector(float4x4 viewMatrix)
{
    // Extract forward vector from the view matrix
    // The third row of the view matrix represents the forward direction in view space
    float3 forward = -float3(viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2]);
    
    return normalize(forward);
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float4 worldPosition = WorldPositionRT.Load(int3(tid, 0));
    float3 normal = NormalRT.Load(int3(tid, 0)).xyz;
    float4 albedo = ColorRT.Load(int3(tid, 0));
    float4 roughnessMetallic = MetalRoughnessRT.Load(int3(tid, 0));

    float depthMousePos = DepthRT.Load(int3(MousePosition, 0)).x;
    if(depthMousePos > 0.999f) //sky
    {
        HoveredMeshes[0] = -1;
    }
    else
    {
        HoveredMeshes[0] = asint(MeshIDRT.Load(int3(MousePosition, 0)).x);
    }
    
    //Combining the lighting and shadowing
    Light light = Lights[0];
    float3 lightDir = -GetLightDirection(light);

    float shadowFactor = CalculateShadowFactor(Shadowmap, cmpSampler, worldPosition, normal, light.View, light.Projection);
    // float3 viewPosition = mul(view, worldPosition).xyz;
    float3 viewDirection = GetForwardVector(view);
    float3 radiance = light.Color * light.Intensity * M_1_PI_F * saturate(dot(lightDir, normal));
    float3 ambient = AMBIENT * albedo.rgb;
    float3 diffuse = CookTorrenceBRDF(normal, viewDirection, lightDir, albedo.rgb, roughnessMetallic.g, roughnessMetallic.b);

    float3 resultColor = ambient + diffuse * radiance * saturate(shadowFactor);
    // float3 resultColor = viewPosition;

    //Writing the result to the render target
    TargetRT[tid] = float4(resultColor, 1.0f);
}