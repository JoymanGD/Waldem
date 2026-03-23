#include "../Shading.hlsl"
#include "../Lighting.hlsl"
#include "../Common.hlsl"
#include "../Types.hlsl"
#include "../Materials.hlsl"
#include "RayTracingCommon.hlsl"

#define DIR_LIGHT_INTENSITY_RATIO 10.0f

struct MaterialAttribute;

struct Payload
{
    float4 Color;
    float3 NextRayOrigin;
    float3 NextRayDirection;
    float3 Throughput;
    bool Missed;
    bool ContinuePath;
    bool CastShadows;
    bool IsReflectionPass;
    bool IsPathTracing;
    uint Depth;
    uint Seed;
};

struct Attributes
{
    float2 barycentrics;
};

struct RayTracingSceneData
{
    float4x4 InvViewMatrix;
    float4x4 InvProjectionMatrix;
    float3 CameraPosition;
    int NumLights;
};

cbuffer RootConstants : register(b0)
{
    uint WorldPositionRTID;
    uint NormalRTID;
    uint ColorRTID;
    uint ORMRTID;
    uint RadianceRTID;
    uint PathTracingRTID;
    uint ReflectionRTID;
    uint LightsBufferID;
    uint LightTransformsBufferID;
    uint LightsIndicesBufferID;
    uint SceneDataBufferID; 
    uint TLASID;
    uint VertexBufferId;
    uint IndexBufferId;
    uint DrawCommandsBufferId;
    uint MaterialBufferId;
    uint EnableReflections;
    uint EnableDirectLighting;
    uint EnableSpecular;
    uint EnableMetallic;
    uint EnablePathTracing;
    uint PathTracingMaxBounces;
    uint PathTracingSamplesPerPixel;
    uint PathTracingFrameIndex;
    uint EnablePathTracingAccumulation;
};

SamplerState myStaticSampler : register(s0);

float smoothstep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

uint Hash(uint state)
{
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

float Random01(inout uint state)
{
    state = Hash(state);
    return (float)(state & 0x00FFFFFFu) / 16777216.0f;
}

float3 SampleCosineHemisphere(float3 n, inout uint seed)
{
    float u1 = Random01(seed);
    float u2 = Random01(seed);

    float r = sqrt(u1);
    float phi = 2.0f * PI * u2;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(saturate(1.0f - u1));

    float3 up = abs(n.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    float3 tangent = normalize(cross(up, n));
    float3 bitangent = cross(n, tangent);
    return normalize(tangent * x + bitangent * y + n * z);
}

float3 SafeNormalize(float3 v, float3 fallback)
{
    float len2 = dot(v, v);
    if(len2 < 1e-12f)
    {
        return fallback;
    }

    return v * rsqrt(len2);
}

float3 GetPathTracingDirectLighting(float3 worldPosition, float3 worldNormal, float3 albedo, bool castShadows, float3 rayOrigin, RayTracingSceneData sceneData)
{
    float3 direct = 0.0f;

    if(EnableDirectLighting == 0)
    {
        return direct;
    }

    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[LightsBufferID];
    StructuredBuffer<float4x4> LightTransforms = ResourceDescriptorHeap[LightTransformsBufferID];
    StructuredBuffer<int> LightIndices = ResourceDescriptorHeap[LightsIndicesBufferID];
    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];

    for (int i = 0; i < sceneData.NumLights; i++)
    {
        int lightIndex = LightIndices[i];
        Light light = Lights[lightIndex];
        float4x4 lightTransform = LightTransforms[lightIndex];
        float3 lightDir = 0.0f;
        float3 radiance = 0.0f;

        if(length(light.Color) == 0.0f)
        {
            light.Color = float3(0.001f, 0.001f, 0.001f);
        }

        if(light.Type == 0)
        {
            lightDir = -GetForwardVector(lightTransform);
            const float NdotL = saturate(dot(worldNormal, lightDir));
            if(NdotL <= 0.0f)
            {
                continue;
            }

            Payload shadowPayload;
            shadowPayload.Color = 0.0f;
            shadowPayload.Missed = false;
            shadowPayload.CastShadows = castShadows;
            shadowPayload.IsReflectionPass = false;
            shadowPayload.IsPathTracing = false;
            shadowPayload.Depth = 0;
            shadowPayload.Seed = 0;

            RayDesc ray;
            ray.Origin = rayOrigin;
            ray.Direction = lightDir;
            ray.TMin = TMIN;
            ray.TMax = TMAX;

            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, shadowPayload);

            if(shadowPayload.Missed || !shadowPayload.CastShadows)
            {
                radiance = light.Color * light.Intensity / DIR_LIGHT_INTENSITY_RATIO * NdotL;
            }
        }
        else if(light.Type == 1)
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            float3 toLight = lightPosition - worldPosition;
            float distance = length(toLight);
            if(distance <= 1e-6f || distance > light.Radius)
            {
                continue;
            }

            lightDir = toLight / distance;
            const float NdotL = saturate(dot(worldNormal, lightDir));
            if(NdotL <= 0.0f)
            {
                continue;
            }

            Payload shadowPayload;
            shadowPayload.Color = 0.0f;
            shadowPayload.Missed = false;
            shadowPayload.CastShadows = castShadows;
            shadowPayload.IsReflectionPass = false;
            shadowPayload.IsPathTracing = false;
            shadowPayload.Depth = 0;
            shadowPayload.Seed = 0;

            RayDesc ray;
            ray.Origin = rayOrigin;
            ray.Direction = lightDir;
            ray.TMin = TMIN;
            ray.TMax = max(min(distance, light.Radius) - TMIN, TMIN);

            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, shadowPayload);

            if(shadowPayload.Missed || !shadowPayload.CastShadows)
            {
                float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);
                attenuation *= smoothstep(light.Radius, 0.0, distance);
                radiance = light.Color * light.Intensity * attenuation * NdotL;
            }
        }
        else if(light.Type == 2)
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            float3 toLight = lightPosition - worldPosition;
            float3 spotLightForward = GetForwardVector(lightTransform);
            float distance = length(toLight);
            if(distance <= 1e-6f || distance > light.Radius)
            {
                continue;
            }

            lightDir = toLight / distance;
            const float NdotL = saturate(dot(worldNormal, lightDir));
            if(NdotL <= 0.0f)
            {
                continue;
            }

            float spotDot = dot(lightDir, normalize(-spotLightForward));
            float spotFalloff = smoothstep(cos(radians(light.OuterCone)), cos(radians(light.InnerCone)), spotDot);
            spotFalloff = pow(spotFalloff, light.Softness);
            if(spotFalloff <= 0.0f)
            {
                continue;
            }

            Payload shadowPayload;
            shadowPayload.Color = 0.0f;
            shadowPayload.Missed = false;
            shadowPayload.CastShadows = castShadows;
            shadowPayload.IsReflectionPass = false;
            shadowPayload.IsPathTracing = false;
            shadowPayload.Depth = 0;
            shadowPayload.Seed = 0;

            RayDesc ray;
            ray.Origin = rayOrigin;
            ray.Direction = lightDir;
            ray.TMin = TMIN;
            ray.TMax = max(min(distance, light.Radius) - TMIN, TMIN);

            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, shadowPayload);

            if(shadowPayload.Missed || !shadowPayload.CastShadows)
            {
                float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);
                attenuation *= smoothstep(light.Radius, 0.0, distance);
                radiance = light.Color * light.Intensity * attenuation * spotFalloff * NdotL;
            }
        }

        direct += (albedo * (1.0f / PI)) * radiance;
    }

    return direct;
}

[shader("raygeneration")]
void RayGenShader()
{
    StructuredBuffer<RayTracingSceneData> SceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];
    RayTracingSceneData sceneData = SceneDataBuffer[0];
    Texture2D WorldPositionsRT = ResourceDescriptorHeap[WorldPositionRTID];
    Texture2D NormalRT = ResourceDescriptorHeap[NormalRTID];
    Texture2D ColorRT = ResourceDescriptorHeap[ColorRTID];
    RWTexture2D<float4> PathTracingRT = ResourceDescriptorHeap[PathTracingRTID];
    RWTexture2D<float4> ReflectionRT = ResourceDescriptorHeap[ReflectionRTID];
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    float3 worldPosition = WorldPositionsRT.Load(uint3(dispatchIndex, 0)).xyz;
    float3 normal = NormalRT.Load(uint3(dispatchIndex, 0)).xyz;
    float4 albedo = ColorRT.Load(uint3(dispatchIndex, 0));
    float3 rayOrigin = worldPosition.xyz + normal * 0.001f;

    bool hasGeometry = length(normal) > 0.0001f;

    if(!hasGeometry)
    {
        float3 sky = float3(0.53f, 0.81f, 0.92f);
        float3 outputColor = sky;
        if(EnablePathTracingAccumulation != 0 && PathTracingFrameIndex > 0)
        {
            float3 prev = PathTracingRT[dispatchIndex].rgb;
            float blend = 1.0f / (PathTracingFrameIndex + 1.0f);
            outputColor = lerp(prev, outputColor, blend);
        }
        PathTracingRT[dispatchIndex] = float4(outputColor, 1.0f);
        ReflectionRT[dispatchIndex] = 0.0f;
        return;
    }

    float3 worldNormal = SafeNormalize(normal, float3(0.0f, 1.0f, 0.0f));
    float3 direct = GetPathTracingDirectLighting(worldPosition, worldNormal, albedo.rgb, true, rayOrigin, sceneData);
    uint spp = max(PathTracingSamplesPerPixel, 1u);
    float3 indirectAccum = 0.0f;

    for(uint s = 0; s < spp; s++)
    {
        uint seed = dispatchIndex.x * 1973u + dispatchIndex.y * 9277u + PathTracingFrameIndex * 26699u + s * 3181u + 911u;
        float3 sampleContribution = 0.0f;

        RayDesc pathRay;
        pathRay.Origin = rayOrigin;
        pathRay.Direction = SampleCosineHemisphere(worldNormal, seed);
        pathRay.TMin = TMIN;
        pathRay.TMax = TMAX;

        Payload pathPayload;
        pathPayload.Color = 0.0f;
        pathPayload.NextRayOrigin = 0.0f;
        pathPayload.NextRayDirection = 0.0f;
        pathPayload.Throughput = albedo.rgb;
        pathPayload.Missed = false;
        pathPayload.ContinuePath = false;
        pathPayload.CastShadows = true;
        pathPayload.IsReflectionPass = false;
        pathPayload.IsPathTracing = true;
        pathPayload.Depth = 1;
        pathPayload.Seed = seed;

        for(uint bounce = 0; bounce < PathTracingMaxBounces; bounce++)
        {
            pathPayload.Color = 0.0f;
            pathPayload.Missed = false;
            pathPayload.ContinuePath = false;
            pathPayload.Depth = bounce + 1;

            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, pathRay, pathPayload);

            sampleContribution += pathPayload.Color.rgb;

            if(pathPayload.Missed || !pathPayload.ContinuePath)
            {
                break;
            }

            pathRay.Origin = pathPayload.NextRayOrigin;
            pathRay.Direction = pathPayload.NextRayDirection;
            pathRay.TMin = TMIN;
            pathRay.TMax = TMAX;
        }

        indirectAccum += sampleContribution;
    }

    float3 tracedColor = direct + (indirectAccum / spp);
    tracedColor = min(tracedColor, float3(16.0f, 16.0f, 16.0f));

    float3 finalPT = tracedColor;
    if(EnablePathTracingAccumulation != 0 && PathTracingFrameIndex > 0)
    {
        float3 prev = PathTracingRT[dispatchIndex].rgb;
        float blend = 1.0f / (PathTracingFrameIndex + 1.0f);
        finalPT = lerp(prev, finalPT, blend);
    }

    PathTracingRT[dispatchIndex] = float4(finalPT, 1.0f);
    ReflectionRT[dispatchIndex] = 0.0f;
}

float4 SampleTexture(Texture2D texture, float2 uv)
{
    // Filtered sampling avoids nearest-neighbor sparkle in traced shading.
    return texture.SampleLevel(myStaticSampler, uv, 0.0f);
}

[shader("miss")]
void MissShader(inout Payload payload)
{
    payload.Missed = true;
    payload.ContinuePath = false;

    if(payload.IsPathTracing || payload.IsReflectionPass)
    {
        // payload.Color = float4(0.53f, 0.81f, 0.92f, 1.0f);
        payload.Color = float4(.3f, .3f, .3f, 1.0f);
    }
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in Attributes attribs)
{
    payload.Missed = false;
    
    uint instanceId = InstanceID();
    
    StructuredBuffer<MaterialAttribute> materialAttributes = ResourceDescriptorHeap[MaterialBufferId];
    MaterialAttribute material = materialAttributes[instanceId];

    payload.CastShadows = material.CastShadows;

    if(payload.IsPathTracing)
    {
        uint primIndex = PrimitiveIndex();

        StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[VertexBufferId];
        StructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[IndexBufferId];
        StructuredBuffer<DrawCommand> drawCommandsBuffer = ResourceDescriptorHeap[DrawCommandsBufferId];
        DrawCommand drawCommand = drawCommandsBuffer[instanceId];
        
        uint3 triIndices = uint3(
            indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 0],
            indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 1],
            indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 2]
        );

        float b0 = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
        float b1 = attribs.barycentrics.x;
        float b2 = attribs.barycentrics.y;

        Vertex v1 = vertexBuffer[drawCommand.BaseVertexLocation + triIndices.x];
        Vertex v2 = vertexBuffer[drawCommand.BaseVertexLocation + triIndices.y];
        Vertex v3 = vertexBuffer[drawCommand.BaseVertexLocation + triIndices.z];

        float4 c0 = v1.Color;
        float4 c1 = v2.Color;
        float4 c2 = v3.Color;
        float4 hitColor = c0 * b0 + c1 * b1 + c2 * b2;

        float4 n0 = v1.Normal;
        float4 n1 = v2.Normal;
        float4 n2 = v3.Normal;
        float4 hitNormal = normalize(n0 * b0 + n1 * b1 + n2 * b2);

        float2 uv0 = v1.UV;
        float2 uv1 = v2.UV;
        float2 uv2 = v3.UV;
        float2 hitUV = uv0 * b0 + uv1 * b1 + uv2 * b2;

        float4 t0 = v1.Tangent;
        float4 t1 = v2.Tangent;
        float4 t2 = v3.Tangent;
        float4 hitTangent = normalize(t0 * b0 + t1 * b1 + t2 * b2);

        float3 bt0 = v1.Bitangent.xyz;
        float3 bt1 = v2.Bitangent.xyz;
        float3 bt2 = v3.Bitangent.xyz;
        float3 hitBitangent = normalize(bt0 * b0 + bt1 * b1 + bt2 * b2);

        float4 color = hitColor * material.Albedo;

        if(material.DiffuseTextureIndex != -1)
        {
            Texture2D ColorTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.DiffuseTextureIndex)];
            float4 sampledColor = SampleTexture(ColorTexture, hitUV);
            color *= sampledColor;
        }

        float4 normal = hitNormal;
        if(material.NormalTextureIndex != -1)
        {
            Texture2D<float4> NormalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.NormalTextureIndex)];
            normal = SampleTexture(NormalTexture, hitUV);
            normal = float4(GetNormal(hitNormal.xyz, hitTangent.xyz, hitBitangent.xyz, normal), 0.0f);
        }

        float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

        StructuredBuffer<RayTracingSceneData> SceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
        RayTracingSceneData sceneData = SceneDataBuffer[0];

        normal = mul(ObjectToWorld4x3(), normal.xyz);
        float3 worldNormal = SafeNormalize(normal.xyz, float3(0.0f, 1.0f, 0.0f));
        if(dot(worldNormal, -WorldRayDirection()) < 0.0f)
        {
            worldNormal = -worldNormal;
        }
        float3 rayOrigin = hitPos + worldNormal * 0.001f;

        float3 direct = GetPathTracingDirectLighting(hitPos, worldNormal, color.rgb, material.CastShadows, rayOrigin, sceneData);
        payload.Color = float4(min(payload.Throughput * direct, float3(16.0f, 16.0f, 16.0f)), 1.0f);
        payload.ContinuePath = false;

        if(payload.Depth < PathTracingMaxBounces)
        {
            payload.Seed = payload.Seed + payload.Depth * 1664525u + 1013904223u;
            payload.Throughput *= color.rgb;
            payload.NextRayOrigin = rayOrigin;
            payload.NextRayDirection = SampleCosineHemisphere(worldNormal, payload.Seed);
            payload.ContinuePath = true;
        }
    }
}



