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
        glm::vec3 Position;
        LightType Type;
        glm::vec3 Direciton;
        float Intensity;
        glm::vec3 Color;
        float Range;
    };
}