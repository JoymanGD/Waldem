#define A0 1.0f
#define A1 0.1f
#define A2 0.01f

struct Light
{
    float3 Color;
    float Intensity;
    uint Type;
    float Radius;
    float InnerCone;
    float OuterCone;
    float Softness;
    float AreaWidth;
    float AreaHeight;
    int AreaTwoSided;
};