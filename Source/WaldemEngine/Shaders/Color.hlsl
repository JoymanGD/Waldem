#define GRADIENT_MAX_POINTS 5

struct Gradient
{
    // xyz = RGB, w = position
    float4 Points[GRADIENT_MAX_POINTS];

    uint Count;
    uint3 Padding;
};

float4 SampleGradient(Gradient gradient, float t)
{
    t = saturate(t);

    if (gradient.Count == 0)
        return float4(1, 1, 1, 1);

    if (gradient.Count == 1 || t <= gradient.Points[0].w)
        return float4(gradient.Points[0].xyz, 1.0f);

    for (uint i = 1; i < gradient.Count; ++i)
    {
        float4 previous = gradient.Points[i - 1];
        float4 current  = gradient.Points[i];

        if (t <= current.w)
        {
            float range = max(current.w - previous.w, 0.00001f);
            float factor = saturate((t - previous.w) / range);

            return float4(lerp(previous.xyz, current.xyz, factor), 1.0f);
        }
    }

    return float4(gradient.Points[gradient.Count - 1].xyz, 1.0f);
}