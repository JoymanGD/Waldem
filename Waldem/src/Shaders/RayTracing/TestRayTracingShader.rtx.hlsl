#include "../Shading.hlsl"
#include "../Lighting.hlsl"
#include "../Common.hlsl"

#define DIR_LIGHT_INTENSITY_RATIO 10.0f

struct Payload
{
    bool Missed;
};

struct Attributes
{
    float2 barycentrics;
};

RaytracingAccelerationStructure g_TLAS : register(t0);
StructuredBuffer<Light> Lights : register(t1);
StructuredBuffer<matrix> LightTransforms : register(t2);
Texture2D WorldPositionsRT : register(t3);
Texture2D NormalRT : register(t4);
Texture2D ColorRT : register(t5);
Texture2D ORMRT : register(t6);
RWTexture2D<float4> OutputColor : register(u0);

cbuffer MyConstantBuffer : register(b0)
{
    matrix InvView;
    matrix InvProj;
    int NumLights;
    //TODO: add lights amount
};

float smoothstep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

[shader("raygeneration")]
void RayGenShader()
{
    float3 radiance = 0.0f;
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    float4 worldPosition = WorldPositionsRT.Load(uint3(dispatchIndex, 0));
    float4 normal = NormalRT.Load(uint3(dispatchIndex, 0));
    float4 albedo = ColorRT.Load(uint3(dispatchIndex, 0));
    float4 orm = ORMRT.Load(uint3(dispatchIndex, 0));

    Payload payload;
            
    //shadow ray
    for (int i = 0; i < NumLights; i++)
    {
        payload.Missed = false;
        Light light = Lights[i];
        matrix lightTransform = LightTransforms[i];
        float3 lightDirection = 0.0f;
        
        if(light.Type == 0) //Directional
        {
            lightDirection = -GetForwardVector(lightTransform);
            float NdotL = saturate(dot(normal.xyz, lightDirection));
            
            RayDesc ray;
            ray.Origin = worldPosition;
            ray.Direction = lightDirection;
            ray.TMin = 0.001;
            ray.TMax = 1000.0;

            TraceRay(g_TLAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 1, 0, ray, payload);

            if(payload.Missed)
            {
                radiance += normalize(light.Color) * light.Intensity / DIR_LIGHT_INTENSITY_RATIO * NdotL;
            }
        }
        else if(light.Type == 1) //Point
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            lightDirection = lightPosition - worldPosition;
            float distance = length(lightDirection);
            
            lightDirection /= distance;
            
            float NdotL = saturate(dot(normal.xyz, lightDirection));
            
            RayDesc ray;
            ray.Origin = worldPosition;
            ray.Direction = lightDirection;
            ray.TMin = 0.001;
            ray.TMax = min(distance, light.Radius);

            TraceRay(g_TLAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 1, 0, ray, payload);

            if(payload.Missed)
            {
                float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);
                attenuation *= smoothstep(light.Radius, 0.0, distance);
                radiance += normalize(light.Color) * light.Intensity * attenuation * NdotL;
            }
        }
        else if(light.Type == 2) //Spot
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            lightDirection = lightPosition - worldPosition;
            float3 spotLightForward = GetForwardVector(lightTransform);
            float distance = length(lightDirection);
            
            lightDirection /= distance;
            
            float NdotL = saturate(dot(normal.xyz, lightDirection));
            
            RayDesc ray;
            ray.Origin = worldPosition;
            ray.Direction = lightDirection;
            ray.TMin = 0.001;
            ray.TMax = min(distance, light.Radius);

            TraceRay(g_TLAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 1, 0, ray, payload);

            if(payload.Missed)
            {
                float spotDot = dot(lightDirection, normalize(-spotLightForward));
                float spotFalloff = smoothstep(cos(radians(light.OuterCone)), cos(radians(light.InnerCone)), spotDot);
                spotFalloff = pow(spotFalloff, light.Sharpness);
                float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);
                attenuation *= smoothstep(light.Radius, 0.0, distance);
                radiance += normalize(light.Color) * light.Intensity * attenuation * spotFalloff * NdotL;
            }
        }
        
        diffuse += GetDiffuseColor(lightDirection, worldPosition, normal, albedo, orm, InvView);
    }
    
    OutputColor[dispatchIndex] = float4(radiance * diffuse, 1.0);
}

[shader("miss")]
void MissShader(inout Payload payload)
{
    payload.Missed = true;
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in Attributes attribs)
{
}