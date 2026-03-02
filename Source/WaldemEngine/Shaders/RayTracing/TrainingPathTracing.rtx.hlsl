#include "../Common.hlsl"
#include "../Types.hlsl"
#include "../Materials.hlsl"
#include "../Lighting.hlsl"
#include "RayTracingCommon.hlsl"

#ifndef PI
#define PI 3.14159265359f
#endif

struct TrainingSample
{
    float4 WorldPosition;
    float4 WorldNormal;
    float4 DiffuseIrradiance;
};

struct Payload
{
    float3 Radiance;
    uint IsShadowRay;
    uint Hit;
    uint Depth;
    uint Seed;
};

struct Attributes
{
    float2 barycentrics;
};

cbuffer RootConstants : register(b0)
{
    uint OutputBufferID;
    uint TLASID;
    uint VertexBufferId;
    uint IndexBufferId;
    uint TriangleRefsBufferId;
    uint TriangleCdfBufferId;
    uint DrawCommandsBufferId;
    uint MaterialBufferId;
    uint WorldTransformsBufferId;
    uint LightsBufferId;
    uint LightTransformsBufferId;
    uint LightsIndicesBufferId;
    uint NumTriangleRefs;
    uint NumLights;
    uint RaysPerPoint;
    uint MaxBounces;
    uint SeedOffset;
};

SamplerState myStaticSampler : register(s0);

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

float smoothstep_custom(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

bool GetRandomSurfaceSample(uint sampleIndex, inout uint seed, out float3 outWorldPos, out float3 outWorldNormal)
{
    StructuredBuffer<uint2> triangleRefs = ResourceDescriptorHeap[TriangleRefsBufferId];
    StructuredBuffer<float> triangleCdf = ResourceDescriptorHeap[TriangleCdfBufferId];
    StructuredBuffer<DrawCommand> drawCommands = ResourceDescriptorHeap[DrawCommandsBufferId];
    StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[VertexBufferId];
    StructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[IndexBufferId];
    StructuredBuffer<float4x4> worldTransforms = ResourceDescriptorHeap[WorldTransformsBufferId];

    if(NumTriangleRefs == 0)
    {
        outWorldPos = 0.0f;
        outWorldNormal = float3(0.0f, 1.0f, 0.0f);
        return false;
    }

    float u = Random01(seed);
    uint left = 0;
    uint right = NumTriangleRefs - 1;
    [loop]
    while(left < right)
    {
        uint mid = (left + right) >> 1;
        if(u <= triangleCdf[mid])
        {
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }
    uint triangleSampleIndex = left;

    uint2 triangleRef = triangleRefs[triangleSampleIndex];
    uint drawId = triangleRef.x;
    uint triId = triangleRef.y;
    DrawCommand command = drawCommands[drawId];

    uint i0 = (uint)((int)indexBuffer[command.StartIndexLocation + triId * 3 + 0] + command.BaseVertexLocation);
    uint i1 = (uint)((int)indexBuffer[command.StartIndexLocation + triId * 3 + 1] + command.BaseVertexLocation);
    uint i2 = (uint)((int)indexBuffer[command.StartIndexLocation + triId * 3 + 2] + command.BaseVertexLocation);

    Vertex v0 = vertexBuffer[i0];
    Vertex v1 = vertexBuffer[i1];
    Vertex v2 = vertexBuffer[i2];

    float bu = Random01(seed);
    float bv = Random01(seed);
    float su = sqrt(bu);
    float b0 = 1.0f - su;
    float b1 = su * (1.0f - bv);
    float b2 = su * bv;

    float3 localPos = v0.Position.xyz * b0 + v1.Position.xyz * b1 + v2.Position.xyz * b2;
    float3 localNormal = SafeNormalize(v0.Normal.xyz * b0 + v1.Normal.xyz * b1 + v2.Normal.xyz * b2, float3(0.0f, 1.0f, 0.0f));
    float4x4 world = worldTransforms[drawId];

    outWorldPos = mul(world, float4(localPos, 1.0f)).xyz;
    outWorldNormal = SafeNormalize(mul((float3x3)world, localNormal), float3(0.0f, 1.0f, 0.0f));
    return true;
}

[shader("raygeneration")]
void RayGenShader()
{
    RWStructuredBuffer<TrainingSample> outputSamples = ResourceDescriptorHeap[OutputBufferID];
    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];

    uint sampleIndex = DispatchRaysIndex().x;
    uint seed = sampleIndex * 1973u + SeedOffset * 9277u + 17u;

    float3 worldPos;
    float3 worldNormal;
    bool validSample = GetRandomSurfaceSample(sampleIndex, seed, worldPos, worldNormal);

    TrainingSample sample = (TrainingSample)0;
    sample.WorldPosition = float4(worldPos, 1.0f);
    sample.WorldNormal = float4(worldNormal, 0.0f);

    if(validSample)
    {
        uint spp = max(RaysPerPoint, 1u);
        float3 accumIncidentRadiance = 0.0f;

        [loop]
        for(uint s = 0; s < spp; s++)
        {
            uint pathSeed = seed + s * 7919u;
            float3 dir = SampleCosineHemisphere(worldNormal, pathSeed);

            RayDesc ray;
            ray.Origin = worldPos + worldNormal * 0.001f;
            ray.Direction = dir;
            ray.TMin = TMIN;
            ray.TMax = TMAX;

            Payload payload;
            payload.Radiance = 0.0f;
            payload.IsShadowRay = 0u;
            payload.Hit = 0u;
            payload.Depth = 0u;
            payload.Seed = pathSeed;

            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);
            accumIncidentRadiance += payload.Radiance;
        }

        // Cosine-weighted hemisphere estimator for irradiance:
        // E = (1/N) * sum( Li * PI )
        float3 irradiance = (accumIncidentRadiance / spp) * PI;
        sample.DiffuseIrradiance = float4(irradiance, 1.0f);
    }

    outputSamples[sampleIndex] = sample;
}

[shader("miss")]
void MissShader(inout Payload payload)
{
    if(payload.IsShadowRay != 0u)
    {
        payload.Hit = 0u;
        payload.Radiance = 0.0f;
        return;
    }

    payload.Hit = 0u;
    payload.Radiance = float3(0.53f, 0.81f, 0.92f);
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in Attributes attribs)
{
    if(payload.IsShadowRay != 0u)
    {
        payload.Hit = 1u;
        payload.Radiance = 0.0f;
        return;
    }

    uint instanceId = InstanceID();

    StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[VertexBufferId];
    StructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[IndexBufferId];
    StructuredBuffer<DrawCommand> drawCommandsBuffer = ResourceDescriptorHeap[DrawCommandsBufferId];
    StructuredBuffer<MaterialAttribute> materialAttributes = ResourceDescriptorHeap[MaterialBufferId];

    DrawCommand drawCommand = drawCommandsBuffer[instanceId];
    MaterialAttribute material = materialAttributes[instanceId];

    uint primIndex = PrimitiveIndex();
    uint3 triIndices = uint3(
        indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 0],
        indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 1],
        indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 2]
    );

    float b0 = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
    float b1 = attribs.barycentrics.x;
    float b2 = attribs.barycentrics.y;

    Vertex v0 = vertexBuffer[(uint)((int)triIndices.x + drawCommand.BaseVertexLocation)];
    Vertex v1 = vertexBuffer[(uint)((int)triIndices.y + drawCommand.BaseVertexLocation)];
    Vertex v2 = vertexBuffer[(uint)((int)triIndices.z + drawCommand.BaseVertexLocation)];

    float3 localNormal = SafeNormalize(v0.Normal.xyz * b0 + v1.Normal.xyz * b1 + v2.Normal.xyz * b2, float3(0.0f, 1.0f, 0.0f));
    float3 worldNormal = SafeNormalize(mul(ObjectToWorld4x3(), float4(localNormal, 0.0f)).xyz, float3(0.0f, 1.0f, 0.0f));
    if(dot(worldNormal, -WorldRayDirection()) < 0.0f)
    {
        worldNormal = -worldNormal;
    }
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    float3 albedo = material.Albedo.rgb;
    if(material.DiffuseTextureIndex != -1)
    {
        float2 uv = v0.UV * b0 + v1.UV * b1 + v2.UV * b2;
        Texture2D colorTex = ResourceDescriptorHeap[NonUniformResourceIndex(material.DiffuseTextureIndex)];
        albedo *= colorTex.SampleLevel(myStaticSampler, uv, 0.0f).rgb;
    }

    float3 directLight = 0.0f;
    if(NumLights > 0u)
    {
        StructuredBuffer<Light> lights = ResourceDescriptorHeap[LightsBufferId];
        StructuredBuffer<float4x4> lightTransforms = ResourceDescriptorHeap[LightTransformsBufferId];
        StructuredBuffer<int> lightIndices = ResourceDescriptorHeap[LightsIndicesBufferId];
        RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];

        [loop]
        for(uint i = 0; i < NumLights; i++)
        {
            int lightIndex = lightIndices[i];
            Light light = lights[lightIndex];
            float4x4 lightTransform = lightTransforms[lightIndex];
            float3 radiance = 0.0f;
            float3 toLight = 0.0f;
            float lightDistance = TMAX;

            if(light.Type == 0u) // Directional
            {
                toLight = -normalize(float3(lightTransform[0][2], lightTransform[1][2], lightTransform[2][2]));
                float nDotL = saturate(dot(worldNormal, toLight));
                if(nDotL <= 0.0f)
                {
                    continue;
                }

                Payload shadowPayload;
                shadowPayload.Radiance = 0.0f;
                shadowPayload.IsShadowRay = 1u;
                shadowPayload.Hit = 0u;
                shadowPayload.Depth = 0u;
                shadowPayload.Seed = 0u;

                RayDesc shadowRay;
                shadowRay.Origin = hitPos + worldNormal * 0.001f;
                shadowRay.Direction = toLight;
                shadowRay.TMin = TMIN;
                shadowRay.TMax = TMAX;
                TraceRay(TLAS, 0, 0xFF, 0, 1, 0, shadowRay, shadowPayload);

                if(shadowPayload.Hit == 0u)
                {
                    radiance = normalize(light.Color) * light.Intensity / 10.0f * nDotL;
                }
            }
            else if(light.Type == 1u || light.Type == 2u) // Point / Spot
            {
                float3 lightPosition = transpose(lightTransform)[3].xyz;
                float3 l = lightPosition - hitPos;
                lightDistance = length(l);
                if(lightDistance <= 1e-6f || lightDistance > light.Radius)
                {
                    continue;
                }
                toLight = l / lightDistance;

                float nDotL = saturate(dot(worldNormal, toLight));
                if(nDotL <= 0.0f)
                {
                    continue;
                }

                float spotFalloff = 1.0f;
                if(light.Type == 2u)
                {
                    float3 spotForward = normalize(float3(lightTransform[0][2], lightTransform[1][2], lightTransform[2][2]));
                    float spotDot = dot(toLight, -spotForward);
                    spotFalloff = smoothstep_custom(cos(radians(light.OuterCone)), cos(radians(light.InnerCone)), spotDot);
                    spotFalloff = pow(spotFalloff, light.Softness);
                    if(spotFalloff <= 0.0f)
                    {
                        continue;
                    }
                }

                Payload shadowPayload;
                shadowPayload.Radiance = 0.0f;
                shadowPayload.IsShadowRay = 1u;
                shadowPayload.Hit = 0u;
                shadowPayload.Depth = 0u;
                shadowPayload.Seed = 0u;

                RayDesc shadowRay;
                shadowRay.Origin = hitPos + worldNormal * 0.001f;
                shadowRay.Direction = toLight;
                shadowRay.TMin = TMIN;
                shadowRay.TMax = lightDistance - 0.001f;
                TraceRay(TLAS, 0, 0xFF, 0, 1, 0, shadowRay, shadowPayload);

                if(shadowPayload.Hit == 0u)
                {
                    float attenuation = 1.0f / (A0 + A1 * lightDistance + A2 * lightDistance * lightDistance);
                    attenuation *= smoothstep_custom(light.Radius, 0.0f, lightDistance);
                    radiance = normalize(light.Color) * light.Intensity * attenuation * nDotL * spotFalloff;
                }
            }

            directLight += albedo * radiance;
        }
    }

    if(payload.Depth >= MaxBounces)
    {
        payload.Radiance = directLight;
        return;
    }

    float3 nextDir = SampleCosineHemisphere(worldNormal, payload.Seed);

    Payload bouncePayload;
    bouncePayload.Radiance = 0.0f;
    bouncePayload.IsShadowRay = 0u;
    bouncePayload.Hit = 0u;
    bouncePayload.Depth = payload.Depth + 1;
    bouncePayload.Seed = payload.Seed;

    RayDesc bounceRay;
    bounceRay.Origin = hitPos + worldNormal * 0.001f;
    bounceRay.Direction = nextDir;
    bounceRay.TMin = TMIN;
    bounceRay.TMax = TMAX;

    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];
    TraceRay(TLAS, 0, 0xFF, 0, 1, 0, bounceRay, bouncePayload);

    // Lambert BRDF with cosine-weighted sampling:
    // f*cos/pdf = albedo
    float3 indirect = albedo * bouncePayload.Radiance;
    payload.Radiance = directLight + indirect;
}
