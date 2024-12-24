#define AMBIENT 0.2f

struct Light
{
    float3 Color;
    float Intensity;
    float2 Padding1;
    uint Type;
    float Range;
    matrix World;
    matrix View;
    matrix Projection;
};

float3 GetLightDirection(Light light)
{
    float3 forward = transpose(light.World)[2].xyz;
    return normalize(forward);
}