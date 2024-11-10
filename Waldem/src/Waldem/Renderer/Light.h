#pragma once
#include "glm/vec3.hpp"
#include "Waldem/Renderer/RenderTarget.h"

namespace Waldem
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
    };
    
    struct LightData
    {
        Vector3 Position;
        LightType Type;
        Vector3 Direction;
        float Intensity;
        Vector3 Color;
        float Range;
        Matrix4 ViewProjection;
    };
    
    struct Light
    {
        LightData Data;
        RenderTarget* Shadowmap;
    };
}