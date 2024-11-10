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
        Vector3 Color;
        float Intensity;
        LightType Type;
        float Range;
        Vector2 Padding1;
        Matrix4 World;
        Matrix4 View;
        Matrix4 Projection;
    };
    
    struct Light
    {
        LightData Data;
        RenderTarget* Shadowmap;
    };
}