cbuffer OceanParameters : register(b0)
{
    uint N;
    uint L;
    float A;
    float V;
    float2 W;
    float G;
    float Damping;
    float WaveHeight;
    float WaveChoppiness;
    float NormalStrength;
    float SimulationSpeed;
}

cbuffer OceanExtraParameters : register(b2)
{
    float Motion;
    float Distortion;
}