#include "../Common.hlsl"
#include "../Types.hlsl"
#include "../Materials.hlsl"
#include "../Lighting.hlsl"
#include "RayTracingCommon.hlsl"

#ifndef PI
#define PI 3.14159265359f
#endif

#ifndef TRAIN_RAY_BIAS
#define TRAIN_RAY_BIAS TMIN
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
    float3 NextRayOrigin;
    float3 NextRayDirection;
    float3 Throughput;
    uint IsShadowRay;
    uint Hit;
    uint FirstHitBackfacing;
    uint ContinuePath;
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
    float SceneMinX;
    float SceneMinY;
    float SceneMinZ;
    float SceneMinW;
    float SceneMaxX;
    float SceneMaxY;
    float SceneMaxZ;
    float SceneMaxW;
    float SurfaceSampleProbability;
    uint EnableBackfaceCulling;
    uint Padding0;
    uint Padding1;
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

float3 SampleUniformSphere(inout uint seed)
{
    const float u = Random01(seed);
    const float v = Random01(seed);
    const float z = 1.0f - 2.0f * u;
    const float r = sqrt(saturate(1.0f - z * z));
    const float phi = 2.0f * PI * v;
    return float3(r * cos(phi), r * sin(phi), z);
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

float3 GetNormal(float3 normal, float3 tangent, float3 bitangent, float4 normalMap)
{
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return mul(normalMap.xyz * 2.0f - 1.0f, TBN);
}

float4 SampleTexture(Texture2D texture, float2 uv)
{
    return texture.SampleLevel(myStaticSampler, uv, 0.0f);
}

float3 GetTrainingPathTracingDirectLighting(float3 worldPosition, float3 worldNormal, float3 albedo, bool castShadows, float3 rayOrigin)
{
    float3 direct = 0.0f;

    if(NumLights == 0u)
    {
        return direct;
    }

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
        float3 lightDir = 0.0f;
        float3 radiance = 0.0f;

        if(length(light.Color) == 0.0f)
        {
            light.Color = float3(0.001f, 0.001f, 0.001f);
        }

        if(light.Type == 0u)
        {
            lightDir = -GetForwardVector(lightTransform);
            float nDotL = saturate(dot(worldNormal, lightDir));
            if(nDotL <= 0.0f)
            {
                continue;
            }

            Payload shadowPayload;
            shadowPayload.Radiance = 0.0f;
            shadowPayload.NextRayOrigin = 0.0f;
            shadowPayload.NextRayDirection = 0.0f;
            shadowPayload.Throughput = 0.0f;
            shadowPayload.IsShadowRay = 1u;
            shadowPayload.Hit = 0u;
            shadowPayload.FirstHitBackfacing = 0u;
            shadowPayload.ContinuePath = 0u;
            shadowPayload.Depth = 0u;
            shadowPayload.Seed = 0u;

            RayDesc shadowRay;
            shadowRay.Origin = rayOrigin;
            shadowRay.Direction = lightDir;
            shadowRay.TMin = TRAIN_RAY_BIAS;
            shadowRay.TMax = TMAX;
            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, shadowRay, shadowPayload);

            if(shadowPayload.Hit == 0u || !castShadows)
            {
                radiance = light.Color * light.Intensity / 10.0f * nDotL;
            }
        }
        else if(light.Type == 1u)
        {
            float3 lightPosition = transpose(lightTransform)[3].xyz;
            float3 toLight = lightPosition - worldPosition;
            float distance = length(toLight);
            if(distance <= 1e-6f || distance > light.Radius)
            {
                continue;
            }

            lightDir = toLight / distance;
            float nDotL = saturate(dot(worldNormal, lightDir));
            if(nDotL <= 0.0f)
            {
                continue;
            }

            Payload shadowPayload;
            shadowPayload.Radiance = 0.0f;
            shadowPayload.NextRayOrigin = 0.0f;
            shadowPayload.NextRayDirection = 0.0f;
            shadowPayload.Throughput = 0.0f;
            shadowPayload.IsShadowRay = 1u;
            shadowPayload.Hit = 0u;
            shadowPayload.FirstHitBackfacing = 0u;
            shadowPayload.ContinuePath = 0u;
            shadowPayload.Depth = 0u;
            shadowPayload.Seed = 0u;

            RayDesc shadowRay;
            shadowRay.Origin = rayOrigin;
            shadowRay.Direction = lightDir;
            shadowRay.TMin = TRAIN_RAY_BIAS;
            shadowRay.TMax = max(min(distance, light.Radius) - TRAIN_RAY_BIAS, TRAIN_RAY_BIAS);
            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, shadowRay, shadowPayload);

            if(shadowPayload.Hit == 0u || !castShadows)
            {
                float attenuation = 1.0f / (A0 + A1 * distance + A2 * distance * distance);
                attenuation *= smoothstep_custom(light.Radius, 0.0f, distance);
                radiance = light.Color * light.Intensity * attenuation * nDotL;
            }
        }
        else if(light.Type == 2u)
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
            float nDotL = saturate(dot(worldNormal, lightDir));
            if(nDotL <= 0.0f)
            {
                continue;
            }

            float spotDot = dot(lightDir, normalize(-spotLightForward));
            float spotFalloff = smoothstep_custom(cos(radians(light.OuterCone)), cos(radians(light.InnerCone)), spotDot);
            spotFalloff = pow(spotFalloff, light.Softness);
            if(spotFalloff <= 0.0f)
            {
                continue;
            }

            Payload shadowPayload;
            shadowPayload.Radiance = 0.0f;
            shadowPayload.NextRayOrigin = 0.0f;
            shadowPayload.NextRayDirection = 0.0f;
            shadowPayload.Throughput = 0.0f;
            shadowPayload.IsShadowRay = 1u;
            shadowPayload.Hit = 0u;
            shadowPayload.FirstHitBackfacing = 0u;
            shadowPayload.ContinuePath = 0u;
            shadowPayload.Depth = 0u;
            shadowPayload.Seed = 0u;

            RayDesc shadowRay;
            shadowRay.Origin = rayOrigin;
            shadowRay.Direction = lightDir;
            shadowRay.TMin = TRAIN_RAY_BIAS;
            shadowRay.TMax = max(min(distance, light.Radius) - TRAIN_RAY_BIAS, TRAIN_RAY_BIAS);
            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, shadowRay, shadowPayload);

            if(shadowPayload.Hit == 0u || !castShadows)
            {
                float attenuation = 1.0f / (A0 + A1 * distance + A2 * distance * distance);
                attenuation *= smoothstep_custom(light.Radius, 0.0f, distance);
                radiance = light.Color * light.Intensity * attenuation * spotFalloff * nDotL;
            }
        }

        direct += (albedo * (1.0f / PI)) * radiance;
    }

    return direct;
}

bool GetRandomSurfaceSample(uint sampleIndex, inout uint seed, out float3 outWorldPos, out float3 outWorldNormal)
{
    StructuredBuffer<uint2> triangleRefs = ResourceDescriptorHeap[TriangleRefsBufferId];
    StructuredBuffer<float> triangleCdf = ResourceDescriptorHeap[TriangleCdfBufferId];
    StructuredBuffer<DrawCommand> drawCommands = ResourceDescriptorHeap[DrawCommandsBufferId];
    StructuredBuffer<MaterialAttribute> materialAttributes = ResourceDescriptorHeap[MaterialBufferId];
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
    float2 hitUV = v0.UV * b0 + v1.UV * b1 + v2.UV * b2;
    float3 hitTangent = SafeNormalize(v0.Tangent.xyz * b0 + v1.Tangent.xyz * b1 + v2.Tangent.xyz * b2, float3(1.0f, 0.0f, 0.0f));
    float3 hitBitangent = SafeNormalize(v0.Bitangent.xyz * b0 + v1.Bitangent.xyz * b1 + v2.Bitangent.xyz * b2, float3(0.0f, 1.0f, 0.0f));

    MaterialAttribute material = materialAttributes[drawId];
    if(material.NormalTextureIndex != -1)
    {
        Texture2D<float4> normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(material.NormalTextureIndex)];
        float4 normalSample = SampleTexture(normalTex, hitUV);
        localNormal = GetNormal(localNormal, hitTangent, hitBitangent, normalSample);
    }

    float4x4 world = worldTransforms[drawId];

    outWorldPos = mul(world, float4(localPos, 1.0f)).xyz;
    outWorldNormal = SafeNormalize(mul((float3x3)world, localNormal), float3(0.0f, 1.0f, 0.0f));
    return true;
}

bool GetRandomVolumeSample(inout uint seed, out float3 outWorldPos, out float3 outWorldNormal)
{
    float3 sceneMin = float3(SceneMinX, SceneMinY, SceneMinZ);
    float3 sceneMax = float3(SceneMaxX, SceneMaxY, SceneMaxZ);
    float3 span = sceneMax - sceneMin;
    if(span.x <= 1e-6f || span.y <= 1e-6f || span.z <= 1e-6f)
    {
        outWorldPos = 0.0f;
        outWorldNormal = float3(0.0f, 1.0f, 0.0f);
        return false;
    }

    outWorldPos = float3(
        lerp(sceneMin.x, sceneMax.x, Random01(seed)),
        lerp(sceneMin.y, sceneMax.y, Random01(seed)),
        lerp(sceneMin.z, sceneMax.z, Random01(seed))
    );
    outWorldNormal = SampleUniformSphere(seed);
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
    bool validSample = false;
    const bool sampleSurface = (NumTriangleRefs > 0u) && (Random01(seed) < SurfaceSampleProbability);
    if(sampleSurface)
    {
        validSample = GetRandomSurfaceSample(sampleIndex, seed, worldPos, worldNormal);
    }
    else
    {
        validSample = GetRandomVolumeSample(seed, worldPos, worldNormal);
    }

    TrainingSample sample = (TrainingSample)0;
    sample.WorldPosition = float4(worldPos, 1.0f);
    sample.WorldNormal = float4(worldNormal, 0.0f);

    if(validSample)
    {
        uint spp = max(RaysPerPoint, 1u);
        float3 accumIncidentRadiance = 0.0f;
        uint firstHitCount = 0u;
        uint backfacingCount = 0u;

        [loop]
        for(uint s = 0; s < spp; s++)
        {
            uint pathSeed = seed + s * 7919u;
            float3 sampleContribution = 0.0f;

            RayDesc ray;
            ray.Origin = worldPos + worldNormal * TRAIN_RAY_BIAS;
            ray.Direction = SampleCosineHemisphere(worldNormal, pathSeed);
            ray.TMin = TRAIN_RAY_BIAS;
            ray.TMax = TMAX;

            Payload payload;
            payload.Radiance = 0.0f;
            payload.NextRayOrigin = 0.0f;
            payload.NextRayDirection = 0.0f;
            payload.Throughput = 1.0f;
            payload.IsShadowRay = 0u;
            payload.Hit = 0u;
            payload.FirstHitBackfacing = 0u;
            payload.ContinuePath = 0u;
            payload.Depth = 0u;
            payload.Seed = pathSeed;

            [loop]
            for(uint bounce = 0; bounce < MaxBounces; bounce++)
            {
                payload.Radiance = 0.0f;
                payload.Hit = 0u;
                payload.ContinuePath = 0u;
                payload.Depth = bounce;

                TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

                if(bounce == 0u && payload.Hit != 0u)
                {
                    firstHitCount++;
                    backfacingCount += payload.FirstHitBackfacing;
                }

                sampleContribution += payload.Radiance;

                if(payload.Hit == 0u || payload.ContinuePath == 0u)
                {
                    break;
                }

                ray.Origin = payload.NextRayOrigin;
                ray.Direction = payload.NextRayDirection;
                ray.TMin = TRAIN_RAY_BIAS;
                ray.TMax = TMAX;
            }

            accumIncidentRadiance += sampleContribution;
        }

        // The paper culls interior samples based on backfacing majority.
        // Apply only to volume samples and only when enough first-hit evidence exists,
        // otherwise this can incorrectly zero out almost the whole dataset.
        if(EnableBackfaceCulling != 0u && !sampleSurface && firstHitCount > (spp / 2u) && backfacingCount * 2u > firstHitCount)
        {
            validSample = false;
        }

        // Cosine-weighted hemisphere estimator for irradiance:
        // E = (1/N) * sum( Li * PI )
        if(validSample)
        {
            float3 irradiance = (accumIncidentRadiance / spp) * PI;
            sample.DiffuseIrradiance = float4(irradiance, 1.0f);
        }
        else
        {
            sample.DiffuseIrradiance = float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    outputSamples[sampleIndex] = sample;
}

[shader("miss")]
void MissShader(inout Payload payload)
{
    if(payload.IsShadowRay != 0u)
    {
        payload.Hit = 0u;
        payload.FirstHitBackfacing = 0u;
        payload.ContinuePath = 0u;
        payload.Radiance = 0.0f;
        return;
    }

    payload.Hit = 0u;
    payload.FirstHitBackfacing = 0u;
    payload.ContinuePath = 0u;
    payload.Radiance = float3(0.3f, 0.3f, 0.3f);
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in Attributes attribs)
{
    if(payload.IsShadowRay != 0u)
    {
        payload.Hit = 1u;
        payload.FirstHitBackfacing = 0u;
        payload.ContinuePath = 0u;
        payload.Radiance = 0.0f;
        return;
    }

    payload.Hit = 1u;

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

    float4 c0 = v0.Color;
    float4 c1 = v1.Color;
    float4 c2 = v2.Color;
    float4 hitColor = c0 * b0 + c1 * b1 + c2 * b2;

    float3 localNormal = SafeNormalize(v0.Normal.xyz * b0 + v1.Normal.xyz * b1 + v2.Normal.xyz * b2, float3(0.0f, 1.0f, 0.0f));
    float2 uv = v0.UV * b0 + v1.UV * b1 + v2.UV * b2;
    float3 hitTangent = SafeNormalize(v0.Tangent.xyz * b0 + v1.Tangent.xyz * b1 + v2.Tangent.xyz * b2, float3(1.0f, 0.0f, 0.0f));
    float3 hitBitangent = SafeNormalize(v0.Bitangent.xyz * b0 + v1.Bitangent.xyz * b1 + v2.Bitangent.xyz * b2, float3(0.0f, 1.0f, 0.0f));

    float4 sampledNormal = float4(localNormal, 0.0f);
    if(material.NormalTextureIndex != -1)
    {
        Texture2D<float4> normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(material.NormalTextureIndex)];
        sampledNormal = SampleTexture(normalTex, uv);
        sampledNormal = float4(GetNormal(localNormal, hitTangent, hitBitangent, sampledNormal), 0.0f);
    }

    sampledNormal = mul(ObjectToWorld4x3(), sampledNormal);
    float3 worldNormalUnflipped = SafeNormalize(sampledNormal.xyz, float3(0.0f, 1.0f, 0.0f));
    if(payload.Depth == 0u)
    {
        payload.FirstHitBackfacing = dot(worldNormalUnflipped, -WorldRayDirection()) < 0.0f ? 1u : 0u;
    }

    float3 worldNormal = worldNormalUnflipped;
    if(dot(worldNormal, -WorldRayDirection()) < 0.0f)
    {
        worldNormal = -worldNormal;
    }
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    float3 albedo = (hitColor * material.Albedo).rgb;
    if(material.DiffuseTextureIndex != -1)
    {
        Texture2D colorTex = ResourceDescriptorHeap[NonUniformResourceIndex(material.DiffuseTextureIndex)];
        albedo *= SampleTexture(colorTex, uv).rgb;
    }

    float3 rayOrigin = hitPos + worldNormal * TRAIN_RAY_BIAS;
    float3 directLight = GetTrainingPathTracingDirectLighting(hitPos, worldNormal, albedo, material.CastShadows, rayOrigin);

    payload.Radiance = min(payload.Throughput * directLight, float3(16.0f, 16.0f, 16.0f));
    payload.ContinuePath = 0u;

    if(payload.Depth < MaxBounces - 1u)
    {
        payload.Seed = payload.Seed + payload.Depth * 1664525u + 1013904223u;
        payload.Throughput *= albedo;
        payload.NextRayOrigin = rayOrigin;
        payload.NextRayDirection = SampleCosineHemisphere(worldNormal, payload.Seed);
        payload.ContinuePath = 1u;
    }
}
