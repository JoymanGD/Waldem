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
    bool IsReflectionPass;
};

struct OffsetsArgs
{
    uint VertexOffset;
    uint IndexOffset;
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
    uint ReflectionRTID;
    uint LightsBufferID;
    uint LightTransformsBufferID;
    uint SceneDataBufferID; 
    uint TLASID;
    uint VertexBufferId;
    uint IndexBufferId;
    uint OffsetsArgsBufferId;
    uint MaterialBufferId;
};

SamplerState myStaticSampler : register(s0);

float smoothstep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

float3 GetRadiance(Payload payload, float3 worldPosition, float3 normal, float4 albedo, float4 orm, float4 reflection, RayTracingSceneData sceneData, float3 rayOrigin, float3 viewDirection)
{
    float3 radiance = 0.0f;
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[LightsBufferID];
    StructuredBuffer<float4x4> LightTransforms = ResourceDescriptorHeap[LightTransformsBufferID];
    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[TLASID];
    
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
            
            float3 diffuse = GetDiffuseColor(lightDirection, normal, albedo, orm, reflection, viewDirection);
            
            float NdotL = saturate(dot(normal, lightDirection));
            
            RayDesc ray;
            ray.Origin = rayOrigin;
            ray.Direction = lightDirection;
            ray.TMin = TMIN;
            ray.TMax = TMAX;
            
            TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

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
                float3 diffuse = GetDiffuseColor(lightDirection, normal, albedo, orm, reflection, viewDirection);
                
                float NdotL = saturate(dot(normal, lightDirection));
                
                RayDesc ray;
                ray.Origin = rayOrigin;
                ray.Direction = lightDirection;
                ray.TMin = TMIN;
                ray.TMax = min(distance, light.Radius);

                TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

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
                float3 diffuse = GetDiffuseColor(lightDirection, normal, albedo, orm, reflection, viewDirection);
                
                float NdotL = saturate(dot(normal, lightDirection));
                
                RayDesc ray;
                ray.Origin = rayOrigin;
                ray.Direction = lightDirection;
                ray.TMin = TMIN;
                ray.TMax = min(distance, light.Radius);

                TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

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
    RWTexture2D<float4> ReflectionRT = ResourceDescriptorHeap[ReflectionRTID];
    Payload payload;
    
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    float3 worldPosition = WorldPositionsRT.Load(uint3(dispatchIndex, 0)).xyz;
    float3 normal = NormalRT.Load(uint3(dispatchIndex, 0)).xyz;
    float4 albedo = ColorRT.Load(uint3(dispatchIndex, 0));
    float4 orm = ORMRT.Load(uint3(dispatchIndex, 0));

    float3 rayOrigin = worldPosition.xyz + normal * 0.001f;

    float3 viewDirection = normalize(worldPosition - sceneData.CameraPosition);
    
    float3 reflectedDirection = reflect(viewDirection, normalize(normal));

    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = reflectedDirection;
    ray.TMin = TMIN;
    ray.TMax = TMAX;

    payload.IsReflectionPass = true;
    
    TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);

    float4 reflectionColor = payload.Color;
    ReflectionRT[dispatchIndex] = reflectionColor;

    payload.IsReflectionPass = false;

    float3 radiance = GetRadiance(payload, worldPosition, normal, albedo, orm, reflectionColor, sceneData, rayOrigin, viewDirection);
    
    RadianceRT[dispatchIndex] = float4(radiance, 1.0);
}

[shader("miss")]
void MissShader(inout Payload payload)
{
    payload.Missed = true;
    payload.Color = float4(0.53f, 0.81f, 0.92f, 1.0f);
}

float4 SampleTexture(Texture2D texture, float2 uv)
{
    uint2 texSize;
    texture.GetDimensions(texSize.x, texSize.y);
    int3 pixelCoord = int3(uv * texSize, 0);
    return texture.Load(pixelCoord);
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in Attributes attribs)
{
    payload.Missed = false;

    if(payload.IsReflectionPass)
    {
        // Geometry data
        uint primIndex = PrimitiveIndex();
        uint instanceId = InstanceID();

        // Access vertex attributes from your bound geometry buffers
        StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[VertexBufferId];
        StructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[IndexBufferId];
        StructuredBuffer<OffsetsArgs> offsetsArgsBuffer = ResourceDescriptorHeap[OffsetsArgsBufferId];
        OffsetsArgs offsets = offsetsArgsBuffer[instanceId];
        
        uint3 triIndices = uint3(
            indexBuffer[offsets.IndexOffset + primIndex * 3 + 0],
            indexBuffer[offsets.IndexOffset + primIndex * 3 + 1],
            indexBuffer[offsets.IndexOffset + primIndex * 3 + 2]
        );

        // Barycentrics
        float b0 = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
        float b1 = attribs.barycentrics.x;
        float b2 = attribs.barycentrics.y;

        // Interpolated attributes
        Vertex v1 = vertexBuffer[offsets.VertexOffset + triIndices.x];
        Vertex v2 = vertexBuffer[offsets.VertexOffset + triIndices.y];
        Vertex v3 = vertexBuffer[offsets.VertexOffset + triIndices.z];
    
        float3 p0 = v1.Position;
        float3 p1 = v2.Position;
        float3 p2 = v3.Position;
        float3 hitPos = p0 * b0 + p1 * b1 + p2 * b2;

        float4 c0 = v1.Color;
        float4 c1 = v2.Color;
        float4 c2 = v3.Color;
        float4 hitColor = c0 * b0 + c1 * b1 + c2 * b2;

        float3 n0 = v1.Normal;
        float3 n1 = v2.Normal;
        float3 n2 = v3.Normal;
        float3 hitNormal = normalize(n0 * b0 + n1 * b1 + n2 * b2);

        float2 uv0 = v1.UV;
        float2 uv1 = v2.UV;
        float2 uv2 = v3.UV;
        float2 hitUV = uv0 * b0 + uv1 * b1 + uv2 * b2;

        float3 t0 = v1.Tangent;
        float3 t1 = v2.Tangent;
        float3 t2 = v3.Tangent;
        float3 hitTangent = normalize(t0 * b0 + t1 * b1 + t2 * b2);

        float3 bt0 = v1.Bitangent;
        float3 bt1 = v2.Bitangent;
        float3 bt2 = v3.Bitangent;
        float3 hitBitangent = normalize(bt0 * b0 + bt1 * b1 + bt2 * b2);

        StructuredBuffer<MaterialAttribute> materialAttributes = ResourceDescriptorHeap[MaterialBufferId];
        MaterialAttribute material = materialAttributes[instanceId];

        float4 color = hitColor * material.Albedo;

        if(material.DiffuseTextureIndex != -1)
        {
            Texture2D ColorTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.DiffuseTextureIndex)];
            float4 sampledColor = SampleTexture(ColorTexture, hitUV);
        
            color *= sampledColor;
        }
    
        float4 normal = float4(0.0f, 1.0f, 0.0f, 1.0f);

        if(material.NormalTextureIndex != -1)
        {
            Texture2D<float4> NormalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.NormalTextureIndex)]; 
            normal = SampleTexture(NormalTexture, hitUV);
            normal = float4(GetNormal(hitNormal, hitTangent, hitBitangent, normal), 0.0f);
        }
        else
        {
            normal = float4(hitNormal, 0.0f);
        }

        float frontFacing = dot(WorldRayDirection(), normal.xyz) < 0.0;
        if(!frontFacing)
        {
            normal = -normal;
        }

        float4 orm = float4(0.0f, material.Roughness, material.Metallic, 0.0f);
    
        if(material.ORMTextureIndex != -1)
        {
            Texture2D ORMTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.ORMTextureIndex)];
            orm *= SampleTexture(ORMTexture, hitUV);
        }
        
        payload.IsReflectionPass = false;

        StructuredBuffer<RayTracingSceneData> SceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferID];
        RayTracingSceneData sceneData = SceneDataBuffer[0];

        float3 rayOrigin = hitPos + normal.xyz * 0.001f;
        float4 reflection = float4(0.f, 0.f, 0.f, 0.f); //dont use reflection here
        float3 reflectedRadiance = GetRadiance(payload, hitPos, normal.xyz, color, orm, reflection, sceneData, rayOrigin, WorldRayDirection());

        payload.Color = float4(reflectedRadiance, 1.0f);
    }
}