#pragma once
#include "glm/vec3.hpp"

namespace Waldem
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
    };
    
    struct Light
    {
        Vector3 Position;
        LightType Type;
        Vector3 Direciton;
        float Intensity;
        Vector3 Color;
        float Range;
    };
}