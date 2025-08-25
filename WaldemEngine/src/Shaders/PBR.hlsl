#include "Core.hlsl"

float NormalDistribution(float3 normal, float3 halfWayVector, float roughnessFactor)
{
    float alpha = roughnessFactor * roughnessFactor;
    float alphaSquare = alpha * alpha;

    float nDotH = saturate(dot(normal, halfWayVector));

    return alphaSquare / (max(PI * pow((nDotH * nDotH * (alphaSquare - 1.0f) + 1.0f), 2.0f), MIN_FLOAT_VALUE));
}

float3 FresnelSchlick(float vDotH, float3 f0)
{
    return f0 + (1.0f - f0) * pow(clamp(1.0f - vDotH, 0.0f, 1.0f), 5.0f);
}

float3 BaseReflectivity(float3 albedo, float metallicFactor)
{
    return lerp(float3(0.04, 0.04, 0.04), albedo, metallicFactor);
}

float SchlickBeckmannGS(float3 normal, float3 x, float roughnessFactor)
{
    float k = roughnessFactor / 2.0f;
    float nDotX = saturate(dot(normal, x));

    return nDotX / (max((nDotX * (1.0f - k) + k), MIN_FLOAT_VALUE));
}

float SmithGeometry(float3 normal, float3 viewDirection, float3 lightDirection, float roughnessFactor)
{
    return SchlickBeckmannGS(normal, viewDirection, roughnessFactor) * SchlickBeckmannGS(normal, lightDirection, roughnessFactor);
}

float3 CookTorrenceBRDF(float3 normal, float3 viewDirection, float3 pixelToLightDirection, float3 albedo, float3 reflection, float roughnessFactor, float metallicFactor)
{
    float3 halfWayVector = normalize(viewDirection + pixelToLightDirection);

    float3 f0 = BaseReflectivity(albedo, metallicFactor);

    // Using cook torrance BRDF for specular lighting.
    float3 fresnel = FresnelSchlick(max(dot(viewDirection, halfWayVector), 0.0f), f0);

    float normalDistribution = NormalDistribution(normal, halfWayVector, roughnessFactor);
    float geometryFunction = SmithGeometry(normal, viewDirection, pixelToLightDirection, roughnessFactor);

    float3 specularBRDF = (normalDistribution * geometryFunction * fresnel) / max(4.0f * saturate(dot(viewDirection, normal)) * saturate(dot(pixelToLightDirection, normal)), MIN_FLOAT_VALUE);
    bool hasReflection = any(reflection);
    float reflectionStrength = saturate(1.0 - roughnessFactor);
    specularBRDF = hasReflection ? lerp(specularBRDF, reflection * fresnel, reflectionStrength) : specularBRDF;

    // Metals have kD as 0.0f, so more metallic a surface is, closes kS ~ 1 and kD ~ 0.
    // Using lambertian model for diffuse light now.
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - fresnel, float3(0.0f, 0.0f, 0.0f), metallicFactor);

    float3 diffuseBRDF = albedo / PI;

    return (kD * diffuseBRDF + specularBRDF);
}