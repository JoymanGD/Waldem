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

struct RayTracingSceneData
{
    float4x4 InvViewMatrix;
    float4x4 InvProjectionMatrix;
    int NumLights;
};

cbuffer RootConstants : register(b0)
{
    uint WorldPositionRTID;
    uint NormalRTID;
    uint ColorRTID;
    uint ORMRTID;
    uint OutputColorRTID;
    uint LightsBufferID;
    uint LightTransformsBufferID;
    uint SceneDataBufferID; 
    uint TLASID;
};

float smoothstep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

[shader("raygeneration")]
void RayGenShader()
{
    RaytracingAccelerationStructure g_TLAS = ResourceDescriptorHeap[TLASID];
    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[LightsBufferID];
    StructuredBuffer<float4x4> LightTransforms = ResourceDescriptorHeap[LightTransformsBufferID];
    StructuredBuffer<RayTracingSceneData> SceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
    RayTracingSceneData sceneData = SceneDataBuffer[0];
    Texture2D WorldPositionsRT = ResourceDescriptorHeap[WorldPositionRTID];
    Texture2D NormalRT = ResourceDescriptorHeap[NormalRTID];
    Texture2D ColorRT = ResourceDescriptorHeap[ColorRTID];
    Texture2D ORMRT = ResourceDescriptorHeap[ORMRTID];
    RWTexture2D<float4> OutputColor = ResourceDescriptorHeap[OutputColorRTID];
    
    float3 radiance = 0.0f;
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    float4 worldPosition = WorldPositionsRT.Load(uint3(dispatchIndex, 0));
    float4 normal = NormalRT.Load(uint3(dispatchIndex, 0));
    float4 albedo = ColorRT.Load(uint3(dispatchIndex, 0));
    float4 orm = ORMRT.Load(uint3(dispatchIndex, 0));

    Payload payload;

    uint RayFlags = RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
            
    //shadow ray
    for (int i = 0; i < sceneData.NumLights; i++)
    {
        payload.Missed = false;
        Light light = Lights[i];
        matrix lightTransform = LightTransforms[i];
        float3 lightDirection;

        //fix for zero color
        if(length(light.Color) == 0.0f)
        {
            light.Color = float3(0.001f, 0.001f, 0.001f);
        }
        
        if(light.Type == 0) //Directional
        {
            lightDirection = -GetForwardVector(lightTransform);
            
            diffuse = GetDiffuseColor(lightDirection, worldPosition, normal, albedo, orm, sceneData.InvViewMatrix);
            
            float NdotL = saturate(dot(normal.xyz, lightDirection));
            
            RayDesc ray;
            ray.Origin = worldPosition;
            ray.Direction = lightDirection;
            ray.TMin = 0.001;
            ray.TMax = 1000.0;

            TraceRay(g_TLAS, RayFlags, 0xFF, 0, 1, 0, ray, payload);

            if(payload.Missed)
            {
                radiance = normalize(light.Color) * light.Intensity / DIR_LIGHT_INTENSITY_RATIO * NdotL;
            }

            finalColor += diffuse * radiance;
        }
        else if(light.Type == 1) //Point
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            lightDirection = lightPosition - worldPosition;
            float distance = length(lightDirection);
            lightDirection /= distance; //Normalize
            
            if(distance <= light.Radius)
            {
                diffuse = GetDiffuseColor(lightDirection, worldPosition, normal, albedo, orm, sceneData.InvViewMatrix);
                
                float NdotL = saturate(dot(normal.xyz, lightDirection));
                
                RayDesc ray;
                ray.Origin = worldPosition;
                ray.Direction = lightDirection;
                ray.TMin = 0.001;
                ray.TMax = min(distance, light.Radius);

                TraceRay(g_TLAS, RayFlags, 0xFF, 0, 1, 0, ray, payload);

                if(payload.Missed)
                {
                    float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);
                    attenuation *= smoothstep(light.Radius, 0.0, distance);
                    radiance = normalize(light.Color) * light.Intensity * attenuation * NdotL;
                    finalColor += diffuse * radiance;
                }
            }
        }
        else if(light.Type == 2) //Spot
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            lightDirection = lightPosition - worldPosition;
            float3 spotLightForward = GetForwardVector(lightTransform);
            float distance = length(lightDirection);
            lightDirection /= distance; //Normalize

            if(distance <= light.Radius)
            {
                diffuse = GetDiffuseColor(lightDirection, worldPosition, normal, albedo, orm, sceneData.InvViewMatrix);
                
                float NdotL = saturate(dot(normal.xyz, lightDirection));
                
                RayDesc ray;
                ray.Origin = worldPosition;
                ray.Direction = lightDirection;
                ray.TMin = 0.001;
                ray.TMax = min(distance, light.Radius);

                TraceRay(g_TLAS, RayFlags, 0xFF, 0, 1, 0, ray, payload);

                if(payload.Missed)
                {
                    float spotDot = dot(lightDirection, normalize(-spotLightForward));
                    float spotFalloff = smoothstep(cos(radians(light.OuterCone)), cos(radians(light.InnerCone)), spotDot);
                    spotFalloff = pow(spotFalloff, light.Softness);
                    float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);
                    attenuation *= smoothstep(light.Radius, 0.0, distance);
                    radiance = normalize(light.Color) * light.Intensity * attenuation * spotFalloff * NdotL;
                    finalColor += diffuse * radiance;
                }
            }
        }
    }
    
    OutputColor[dispatchIndex] = float4(finalColor, 1.0);
}

[shader("miss")]
void MissShader(inout Payload payload)
{
    payload.Missed = true;
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in Attributes attribs)
{
    payload.Missed = false;
}