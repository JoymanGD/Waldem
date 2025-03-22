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
        //Directional
        Light(Vector3 color, float intensity)
        {
            Color = color;
            Intensity = intensity;
            Type = LightType::Directional;
        }

        //Point
        Light(Vector3 color, float intensity, float radius)
        {
            Color = color;
            Intensity = intensity;
            Type = LightType::Point;
            Radius = radius;
        }

        //Spot
        Light(Vector3 color, float intensity, float radius, float outerCone, float softness)
        {
            Color = color;
            Intensity = intensity;
            Type = LightType::Spot;
            Radius = radius;
            InnerCone = 1.0f;
            OuterCone = outerCone;
            Softness = softness;
        }
        
        Vector3 Color;
        float Intensity;
        LightType Type;
        float Radius;
        float InnerCone = 1; 
        float OuterCone;
        float Softness;
        float AreaWidth;
        float AreaHeight;
        bool AreaTwoSided;
    };
}