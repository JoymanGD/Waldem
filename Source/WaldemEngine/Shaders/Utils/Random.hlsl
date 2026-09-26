float Random(float time, float seed)
{
    return frac(sin(dot(seed * time, float2(12.9898, 78.233))) * 43758.5453);
}

float RandomRange(float time, float seed, float minValue, float maxValue)
{
    return lerp(minValue, maxValue, Random(time, seed));
}

float2 RandomInCircle(float time, float seed, float radius)
{
    float angle = Random(time, seed) * 6.28318530718;
    float r = sqrt(Random(time, seed + 1.0)) * radius;

    return float2(cos(angle), sin(angle)) * r;
}