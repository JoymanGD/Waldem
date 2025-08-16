#pragma once

namespace Waldem
{
    struct WALDEM_API Ocean
    {
        uint N = 512;
        uint L = 1000;
        float A = 2.0f; //Amplitude scaling factor
        float V = 31.0f; //Wind speed
        Vector2 W = Vector2(1.f, 0.f); //Wind direction
        float G = 9.80665f; //Gravitational constant
        float Damping = 1.f;
        float WaveHeight = 3.0f;
        float WaveChoppiness = 2.f;
        float NormalStrength = 500.f;
        float SimulationSpeed = 3.0f;
        Point2 PatchesGridSize = Point2(1, 1);
    };
}
