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
    };
    
    COMPONENT()
    struct Light
    {        
        FIELD()
        Vector3 Color = Vector3(1, 1, 1);
        FIELD()
        float Intensity = 10;
        FIELD()
        float Radius = 10;
        FIELD()
        float InnerCone = 1; 
        FIELD()
        float OuterCone = 45;
        FIELD()
        float Softness = 0.001f;
        FIELD()
        float AreaWidth = 1;
        FIELD()
        float AreaHeight = 1;
        FIELD()
        int AreaTwoSided = false;
        FIELD()
        LightType Type = LightType::Directional;
        
        Light() {}
        
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
    };
}
#include "Light.generated.h"