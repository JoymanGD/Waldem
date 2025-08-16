#pragma once

#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
        Area = 3,
    };

    struct LightData
    {
        Vector3 Color = Vector3(1, 1, 1);
        float Intensity = 10;
        float Radius = 10;
        float InnerCone = 1; 
        float OuterCone = 45;
        float Softness = 0.001f;
        float AreaWidth = 1;
        float AreaHeight = 1;
        int AreaTwoSided = false;
        LightType Type = LightType::Directional;
    };
    
    struct Light
    {
        COMPONENT(Light)
            FIELD(Vector3, Color)
            FIELD(float, Intensity)
            FIELD(float, Radius)
            FIELD(float, InnerCone)
            FIELD(float, OuterCone)
            FIELD(float, Softness)
            FIELD(float, AreaWidth)
            FIELD(float, AreaHeight)
            FIELD(bool, AreaTwoSided)
            FIELD(LightType, Type)
        END_COMPONENT()
        
        //Directional
        Light(Vector3 color, float intensity)
        {
            Data.Color = color;
            Data.Intensity = intensity;
            Data.Type = LightType::Directional;
        }

        //Point
        Light(Vector3 color, float intensity, float radius)
        {
            Data.Color = color;
            Data.Intensity = intensity;
            Data.Type = LightType::Point;
            Data.Radius = radius;
        }

        //Spot
        Light(Vector3 color, float intensity, float radius, float outerCone, float softness)
        {
            Data.Color = color;
            Data.Intensity = intensity;
            Data.Type = LightType::Spot;
            Data.Radius = radius;
            Data.InnerCone = 1.0f;
            Data.OuterCone = outerCone;
            Data.Softness = softness;
        }

        LightData Data = {};
    };
}