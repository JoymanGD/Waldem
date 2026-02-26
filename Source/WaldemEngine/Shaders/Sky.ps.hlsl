#pragma shader_model 6_6

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

struct SceneData
{
    float4x4 InverseProjection;
    float4x4 InverseView;
    float4x4 ViewProjection;
    float4 SkyZenithColor;
    float4 SkyHorizonColor;
    float4 GroundColor;
    float4 SunDirection;
    float4 CameraPosition;
};

SamplerState myStaticSampler : register(s0);

cbuffer RootConstants : register(b0)
{
    uint SceneDataBufferId;
}

float3 GetWorldDir(float2 uv, float4x4 InverseProjection, float4x4 InverseView)
{
    float2 ndc = uv * 2.0f - 1.0f;

    float4 clip = float4(ndc, 1.0f, 1.0f);

    float4 view = mul(InverseProjection, clip);
    view /= view.w;

    float4 world = mul(InverseView, float4(view.xyz, 0.0f));
    // float3 world = normalize(view.xyz);

    return normalize(world);
}

float GridLinesAA(float2 p, float2 ddx, float2 ddy, float thickness)
{
    // pixel footprint in grid space
    float2 w = max(abs(ddx), abs(ddy)) + 1e-6;

    // distance to nearest integer grid line
    float2 d = 0.5 - abs(frac(p) - 0.5);

    // smoother step across footprint → anti-alias lines
    float2 a = saturate((d - thickness * 0.5) / w);
    float2 b = saturate((d + thickness * 0.5) / w);

    // "coverage" between edges
    float2 coverage = b - a;

    // combine X and Y
    return max(coverage.x, coverage.y);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    StructuredBuffer<SceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferId];
    SceneData sceneData = sceneDataBuffer[0];

    float2 uv = input.UV;
    uv.y = 1.0f - uv.y;
    float3 worldDir = GetWorldDir(uv, sceneData.InverseProjection, sceneData.InverseView);

    float t = saturate(worldDir.y * 0.5f + 0.5f);
    float3 sky = lerp(sceneData.SkyHorizonColor.xyz, sceneData.SkyZenithColor.xyz, pow(t, 0.8f));

    float groundBlend = smoothstep(0.0f, 0.02f, worldDir.y);
    float3 finalColor = lerp(sceneData.GroundColor.xyz, sky, groundBlend);

    float sunAmount = max(dot(worldDir, normalize(sceneData.SunDirection.xyz)), 0.0f);
    float3 sunColor = float3(1.0f, 0.95f, 0.8f) * pow(sunAmount, 200.0f);
    finalColor += sunColor;

    return float4(finalColor, 1.0f);
}