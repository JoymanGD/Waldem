SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint MeshIDRTID;
    uint RadianceRTID;
    uint PathTracingRTID;
    uint ColorRTID;
    uint TargetRTID;
    uint NIVIrradianceRTID;
    uint NIVPredictedBufferID;
    uint SkyColorRTID;
    uint EnableSky;
    uint EnablePathTracing;
    uint EnableNIVInference;
    uint HasNIVPrediction;
    uint EnableNIVSpatialFilter;
    float NIVSpatialFilterStrength;
};

float3 LoadNIVPrediction(StructuredBuffer<float4> predictedBuffer, uint2 coord, uint2 screenSize)
{
    if (coord.x >= screenSize.x || coord.y >= screenSize.y)
    {
        return 0.0f;
    }

    const uint linearIndex = coord.y * screenSize.x + coord.x;
    return max(predictedBuffer[linearIndex].rgb, 0.0f);
}

float3 FilterNIVPrediction(StructuredBuffer<float4> predictedBuffer, uint2 tid, uint2 screenSize, Texture2D<int2> meshIDsRT)
{
    float3 center = LoadNIVPrediction(predictedBuffer, tid, screenSize);
    if (EnableNIVSpatialFilter == 0)
    {
        return center;
    }

    const int2 offsets[4] =
    {
        int2(-1, 0),
        int2(1, 0),
        int2(0, -1),
        int2(0, 1)
    };

    float3 accum = center * 2.0f;
    float weightSum = 2.0f;
    const float centerLum = dot(center, float3(0.2126f, 0.7152f, 0.0722f));

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        int2 sampleCoord = int2(tid) + offsets[i];
        if (sampleCoord.x < 0 || sampleCoord.y < 0 || sampleCoord.x >= (int)screenSize.x || sampleCoord.y >= (int)screenSize.y)
        {
            continue;
        }

        if (meshIDsRT.Load(int3(sampleCoord, 0)).g <= 0)
        {
            continue;
        }

        float3 sampleValue = LoadNIVPrediction(predictedBuffer, (uint2)sampleCoord, screenSize);
        const float sampleLum = dot(sampleValue, float3(0.2126f, 0.7152f, 0.0722f));
        const float lumDelta = abs(sampleLum - centerLum);
        const float weight = 1.0f / (1.0f + lumDelta * 4.0f);
        accum += sampleValue * weight;
        weightSum += weight;
    }

    float3 filtered = accum / max(weightSum, 1e-5f);
    return lerp(center, filtered, saturate(NIVSpatialFilterStrength));
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    Texture2D<int2> meshIDsRT = ResourceDescriptorHeap[MeshIDRTID];
    Texture2D radianceRT = ResourceDescriptorHeap[RadianceRTID];
    Texture2D pathTracingRT = ResourceDescriptorHeap[PathTracingRTID];
    Texture2D<float4> colorRT = ResourceDescriptorHeap[ColorRTID];
    Texture2D skyColorRT = ResourceDescriptorHeap[SkyColorRTID];
    RWTexture2D<float4> TargetRT = ResourceDescriptorHeap[TargetRTID];
    RWTexture2D<float4> NIVIrradianceRT = ResourceDescriptorHeap[NIVIrradianceRTID];
    StructuredBuffer<float4> nivPredictedBuffer = ResourceDescriptorHeap[NIVPredictedBufferID];

    int2 meshTypeId = meshIDsRT.Load(int3(tid, 0));
    
    float4 skyColor = skyColorRT.Load(int3(tid,0));

    float3 finalColor;
    float3 nivIrradiance = 0.0f;
    float2 screenSize;
    TargetRT.GetDimensions(screenSize.x, screenSize.y);
    if (tid.x >= (uint)screenSize.x || tid.y >= (uint)screenSize.y)
    {
        return;
    }
    const uint linearIndex = (uint)tid.y * (uint)screenSize.x + (uint)tid.x;
    if (meshTypeId.g > 0)
    {
        finalColor = EnablePathTracing != 0 ? pathTracingRT.Load(uint3(tid, 0)).rgb : radianceRT.Load(uint3(tid, 0));
    }
    else
    {
        finalColor = EnablePathTracing != 0 ? pathTracingRT.Load(uint3(tid, 0)).rgb : (EnableSky != 0 ? skyColor.rgb : 0.0f);
    }

    if (HasNIVPrediction != 0)
    {
        nivIrradiance = FilterNIVPrediction(nivPredictedBuffer, tid, (uint2)screenSize, meshIDsRT);
    }

    NIVIrradianceRT[tid] = float4(nivIrradiance, 1.0f);

    if (EnableNIVInference != 0 && meshTypeId.g > 0)
    {
        const float3 albedo = saturate(colorRT.Load(int3(tid, 0)).rgb);
        const float3 diffuseGI = (albedo * nivIrradiance) * (1.0f / 3.14159265f);
        finalColor += diffuseGI;
    }

    TargetRT[tid] = float4(finalColor, 1.0f);
}
