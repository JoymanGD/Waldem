#define CURVE_MAX_POINTS 5

struct Curve
{
    float2 Points[CURVE_MAX_POINTS];

    float2 Min;
    float2 Max;
    uint Count;
    uint3 Padding;
};

float SampleCurve(Curve curve, float t)
{
    uint count = min(curve.Count, (uint)CURVE_MAX_POINTS);

    if(count == 0)
        return 1.f;

    if(count == 1)
        return curve.Points[0].y;

    if(t <= curve.Points[0].x)
        return curve.Points[0].y;

    for(uint i = 1; i < count; i++)
    {
        float2 previous = curve.Points[i - 1];
        float2 current = curve.Points[i];

        if(t <= current.x)
        {
            float range = current.x - previous.x;

            if(range <= 0.00001f)
                return current.y;

            float factor = saturate((t - previous.x) / range);
            factor = factor * factor * (3.f - 2.f * factor);
            return lerp(previous.y, current.y, factor);
        }
    }

    return curve.Points[count - 1].y;
}