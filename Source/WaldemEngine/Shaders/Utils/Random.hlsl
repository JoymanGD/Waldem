uint Hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

float Random(uint seed)
{
    return (Hash(seed) & 0x00FFFFFFu) / 16777216.0f;
}

float RandomRange(float seed, float minValue, float maxValue)
{
    return lerp(minValue, maxValue, Random(seed));
}

float2 RandomInCircle(uint seed, float radius)
{
    float angle = Random(seed) * 6.28318530718f;
    float r = sqrt(Random(seed + 1u)) * radius;

    return float2(cos(angle), sin(angle)) * r;
}