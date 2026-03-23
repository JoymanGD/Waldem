#pragma shader_model 6_6

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

SamplerState myStaticSampler : register(s0);

cbuffer ScreenQuadRootConstants : register(b0)
{
    uint TargetRTId;
    uint ViewMode;
    uint PathTracingEnabled;
}

float3 HashColor(uint id)
{
    float3 p = float3(id * 0.1031f, id * 0.11369f, id * 0.13787f);
    p = frac(p);
    p += dot(p, p.yzx + 19.19f);
    return frac((p.xxy + p.yzz) * p.zyx);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    if(ViewMode == 5) // MeshID visualization
    {
        Texture2D<int2> MeshIdRT = ResourceDescriptorHeap[TargetRTId];
        int2 pixelPos = int2(input.Position.xy);
        int2 meshTypeId = MeshIdRT.Load(int3(pixelPos, 0));
        if(meshTypeId.g <= 0)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        uint meshId = (uint)meshTypeId.g;
        return float4(HashColor(meshId), 1.0f);
    }

    Texture2D<float4> TargetRT = ResourceDescriptorHeap[TargetRTId];
    float4 color = TargetRT.Sample(myStaticSampler, input.UV);

    if(ViewMode == 1) // Normals
    {
        float3 normal = normalize(color.xyz);
        return float4(normal * 0.5f + 0.5f, 1.0f);
    }

    if(ViewMode == 4) // ORM
    {
        return float4(saturate(color.rgb), 1.0f);
    }

    if(ViewMode == 8) // NIV Irradiance visualization
    {
        int2 pixelPos = int2(input.Position.xy);
        float3 niv = TargetRT.Load(int3(pixelPos, 0)).rgb;

        if(any(isnan(niv)))
        {
            return float4(1.0f, 0.0f, 1.0f, 1.0f);
        }

        // Robust debug view for HDR irradiance: log compression + per-channel mapping.
        float3 hdr = max(niv, 0.0f);
        float3 mapped = log2(1.0f + hdr * 16.0f) / 5.0f;
        return float4(saturate(mapped), 1.0f);
    }

    if(ViewMode == 9) // Path Tracing visualization
    {
        if(PathTracingEnabled == 0u)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        return color;
    }

    return color;
}
