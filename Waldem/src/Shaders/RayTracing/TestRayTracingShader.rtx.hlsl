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
    
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    float4 origin = WorldPositionsRT.Load(uint3(dispatchIndex, 0));
    float4 normal = NormalRT.Load(uint3(dispatchIndex, 0));

    Payload payload;
    payload.Missed = false;
            
    //shadow ray
    for (int i = 0; i < NumLights; i++)
    {
        Light light = Lights[i];
        matrix lightTransform = LightTransforms[i];
        
        if(light.Type == 0) //Directional
        {
            float3 direction = -GetForwardVector(lightTransform);
            float NdotL = saturate(dot(normal.xyz, direction));
            
            RayDesc ray;
            ray.Origin = origin;
            ray.Direction = direction;
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
            float3 direction = lightPosition - origin;
            float NdotL = saturate(dot(normal.xyz, direction));
            float distance = length(direction);
            
            RayDesc ray;
            ray.Origin = origin;
            ray.Direction = normalize(direction);
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
    }
    
    OutputColor[dispatchIndex] = float4(radiance, 1.0);
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