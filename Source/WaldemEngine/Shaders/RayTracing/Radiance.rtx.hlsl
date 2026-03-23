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
    bool Missed;
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

float3 GetRadiance(Payload payload, float3 worldPosition, float3 normal, float4 albedo, float4 orm, float4 reflection, RayTracingSceneData sceneData, float3 rayOrigin, float3 viewDirection)
{
    float3 radiance = 0.0f;
    float4 shadingORM = orm;

    if(EnableMetallic == 0)
    {
        shadingORM.b = 0.0f;
    }

    float3 finalColor = albedo.rgb * AMBIENT;
    
    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[LightsBufferID];
    StructuredBuffer<float4x4> LightTransforms = ResourceDescriptorHeap[LightTransformsBufferID];
    StructuredBuffer<int> LightIndices = ResourceDescriptorHeap[LightsIndicesBufferID];
    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];
    
    //shadow ray
    if(EnableDirectLighting != 0)
    {
        for (int i = 0; i < sceneData.NumLights; i++)
        {
            radiance = 0.0f;
            payload.Missed = false;
            int lightIndex = LightIndices[i];
            Light light = Lights[lightIndex];
            matrix lightTransform = LightTransforms[lightIndex];
            float3 lightDirection;

            //fix for zero color
            if(length(light.Color) == 0.0f)
            {
                light.Color = float3(0.001f, 0.001f, 0.001f);
            }
        
            if(light.Type == 0) //Directional
            {
                lightDirection = -GetForwardVector(lightTransform);
            
                float3 diffuse = GetDiffuseColor(lightDirection, normal, albedo, shadingORM, reflection, viewDirection, EnableSpecular != 0, EnableReflections != 0);
            
                float NdotL = saturate(dot(normal, lightDirection));
            
                RayDesc ray;
                ray.Origin = rayOrigin;
                ray.Direction = lightDirection;
                ray.TMin = TMIN;
                ray.TMax = TMAX;
            
                TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

                if(payload.Missed || !payload.CastShadows)
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
                    float3 diffuse = GetDiffuseColor(lightDirection, normal, albedo, shadingORM, reflection, viewDirection, EnableSpecular != 0, EnableReflections != 0);
                
                    float NdotL = saturate(dot(normal, lightDirection));
                
                    RayDesc ray;
                    ray.Origin = rayOrigin;
                    ray.Direction = lightDirection;
                    ray.TMin = TMIN;
                    ray.TMax = min(distance, light.Radius);

                    TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

                    if(payload.Missed || !payload.CastShadows)
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
                    float3 diffuse = GetDiffuseColor(lightDirection, normal, albedo, shadingORM, reflection, viewDirection, EnableSpecular != 0, EnableReflections != 0);
                
                    float NdotL = saturate(dot(normal, lightDirection));
                
                    RayDesc ray;
                    ray.Origin = rayOrigin;
                    ray.Direction = lightDirection;
                    ray.TMin = TMIN;
                    ray.TMax = min(distance, light.Radius);

                    TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

                    if(payload.Missed || !payload.CastShadows)
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
    }

    return finalColor;
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
    Texture2D ORMRT = ResourceDescriptorHeap[ORMRTID];
    RWTexture2D<float4> RadianceRT = ResourceDescriptorHeap[RadianceRTID];
    RWTexture2D<float4> PathTracingRT = ResourceDescriptorHeap[PathTracingRTID];
    RWTexture2D<float4> ReflectionRT = ResourceDescriptorHeap[ReflectionRTID];
    Payload payload;
    
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    float3 worldPosition = WorldPositionsRT.Load(uint3(dispatchIndex, 0)).xyz;
    float3 normal = NormalRT.Load(uint3(dispatchIndex, 0)).xyz;
    float4 albedo = ColorRT.Load(uint3(dispatchIndex, 0));
    float4 orm = ORMRT.Load(uint3(dispatchIndex, 0));

    float3 rayOrigin = worldPosition.xyz + normal * 0.001f;

    float3 viewDirection = normalize(worldPosition - sceneData.CameraPosition);

    bool hasGeometry = length(normal) > 0.0001f;

    float3 reflectedDirection = reflect(viewDirection, normalize(normal));

    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = reflectedDirection;
    ray.TMin = TMIN;
    ray.TMax = TMAX;

    payload.Color = 0.0f;
    payload.Missed = false;
    payload.CastShadows = true;
    payload.IsReflectionPass = true;
    payload.IsPathTracing = false;
    payload.Depth = 0;
    payload.Seed = 0;

    float roughness = saturate(orm.g);
    float4 reflectionColor = 0.0f;

    // One-ray reflections on rough surfaces become fireflies, so disable at very high roughness.
    if (EnableReflections != 0 && roughness < 0.95f)
    {
        TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

        float oneMinusRoughness = saturate(1.0f - roughness);
        float reflectionMask = oneMinusRoughness * oneMinusRoughness;
        reflectionMask *= reflectionMask; // pow4 falloff

        reflectionColor = payload.Color;
        reflectionColor.rgb *= reflectionMask;
        reflectionColor.rgb = min(reflectionColor.rgb, float3(8.0f, 8.0f, 8.0f));
    }

    ReflectionRT[dispatchIndex] = reflectionColor;

    payload.IsReflectionPass = false;
    payload.IsPathTracing = false;

    float3 radiance = GetRadiance(payload, worldPosition, normal, albedo, orm, reflectionColor, sceneData, rayOrigin, -viewDirection);
    
    RadianceRT[dispatchIndex] = float4(radiance, 1.0);
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

    if(payload.IsPathTracing || payload.IsReflectionPass)
    {
        payload.Color = float4(0.53f, 0.81f, 0.92f, 1.0f);
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

    if(payload.IsReflectionPass)
    {
        // Geometry data
        uint primIndex = PrimitiveIndex();

        // Access vertex attributes from your bound geometry buffers
        StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[VertexBufferId];
        StructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[IndexBufferId];
        StructuredBuffer<DrawCommand> drawCommandsBuffer = ResourceDescriptorHeap[DrawCommandsBufferId];
        DrawCommand drawCommand = drawCommandsBuffer[instanceId];
        
        uint3 triIndices = uint3(
            indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 0],
            indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 1],
            indexBuffer[drawCommand.StartIndexLocation + primIndex * 3 + 2]
        );

        // Barycentrics
        float b0 = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
        float b1 = attribs.barycentrics.x;
        float b2 = attribs.barycentrics.y;

        // Interpolated attributes
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

        float3 bt0 = v1.Bitangent;
        float3 bt1 = v2.Bitangent;
        float3 bt2 = v3.Bitangent;
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

        // float frontFacing = dot(WorldRayDirection(), normal.xyz) < 0.0;
        // if(!frontFacing)
        // {
        //     normal = -normal;
        // }

        float4 orm = float4(0.0f, material.Roughness, material.Metallic, 0.0f);
    
        if(material.ORMTextureIndex != -1)
        {
            Texture2D ORMTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.ORMTextureIndex)];
            orm *= SampleTexture(ORMTexture, hitUV);
        }
        
        payload.IsReflectionPass = false;

        float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

        StructuredBuffer<RayTracingSceneData> SceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
        RayTracingSceneData sceneData = SceneDataBuffer[0];

        normal = mul(ObjectToWorld4x3(), normal);
        float3 rayOrigin = hitPos + normal.xyz * 0.001f;
        float4 reflection = 0; //dont use reflection here
        float3 reflectedRadiance = GetRadiance(payload, hitPos, normal.xyz, color, orm, reflection, sceneData, rayOrigin, -WorldRayDirection());

        payload.Color = float4(reflectedRadiance, 1.0f);
    }
    else if(payload.IsPathTracing)
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

        float3 bt0 = v1.Bitangent;
        float3 bt1 = v2.Bitangent;
        float3 bt2 = v3.Bitangent;
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

        float4 orm = float4(0.0f, material.Roughness, material.Metallic, 0.0f);
        if(material.ORMTextureIndex != -1)
        {
            Texture2D ORMTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.ORMTextureIndex)];
            orm *= SampleTexture(ORMTexture, hitUV);
        }

        if(EnableMetallic == 0)
        {
            orm.b = 0.0f;
        }

        float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

        StructuredBuffer<RayTracingSceneData> SceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
        RayTracingSceneData sceneData = SceneDataBuffer[0];

        normal = mul(ObjectToWorld4x3(), normal);
        float3 worldNormal = normalize(normal.xyz);
        float3 rayOrigin = hitPos + worldNormal * 0.001f;

        Payload directPayload;
        directPayload.Color = 0.0f;
        directPayload.Missed = false;
        directPayload.CastShadows = material.CastShadows;
        directPayload.IsReflectionPass = false;
        directPayload.IsPathTracing = false;
        directPayload.Depth = 0;
        directPayload.Seed = payload.Seed;

        float3 direct = GetRadiance(directPayload, hitPos, worldNormal, color, orm, 0.0f, sceneData, rayOrigin, -WorldRayDirection());
        float3 indirect = 0.0f;

        if(payload.Depth < PathTracingMaxBounces)
        {
            uint seed = payload.Seed + payload.Depth * 1664525u + 1013904223u;
            float3 bounceDir = SampleCosineHemisphere(worldNormal, seed);
            float ndotl = saturate(dot(worldNormal, bounceDir));

            Payload bouncePayload;
            bouncePayload.Color = 0.0f;
            bouncePayload.Missed = false;
            bouncePayload.CastShadows = true;
            bouncePayload.IsReflectionPass = false;
            bouncePayload.IsPathTracing = true;
            bouncePayload.Depth = payload.Depth + 1;
            bouncePayload.Seed = seed;

            RayDesc bounceRay;
            bounceRay.Origin = rayOrigin;
            bounceRay.Direction = bounceDir;
            bounceRay.TMin = TMIN;
            bounceRay.TMax = TMAX;

            RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];
            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, bounceRay, bouncePayload);

            indirect = bouncePayload.Color.rgb * color.rgb;
        }

        float3 traced = direct + indirect;
        payload.Color = float4(min(traced, float3(16.0f, 16.0f, 16.0f)), 1.0f);
    }
}




