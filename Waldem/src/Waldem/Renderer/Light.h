#pragma once
#include "Renderer.h"
#include "Waldem/Renderer/RenderTarget.h"

namespace Waldem
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
        Area = 3,
    };

    struct LightTransformData
    {
        LightType Type;
        Vector3 Forward;
        Vector4 Position;
    };
    
    struct Light
    {
        Light(Vector3 color, float intensity, LightType type, float radius)
        {
            Color = color;
            Intensity = intensity;
            Type = type;
            Radius = radius;
        }
        
        Vector3 Color;
        float Intensity;
        LightType Type;
        float Radius;
        Vector2 Padding1;
    };
}