#pragma once
#include "Waldem/ECS/Component.h"

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
        LightType Type = LightType::Directional;
        float Radius = 10;
        float InnerCone = 1; 
        float OuterCone = 45;
        float Softness = 0.001f;
        float AreaWidth = 1;
        float AreaHeight = 1;
        int AreaTwoSided = false;
    };
    
    struct Light : IComponent<Light>
    {
        Light() = default;
        
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

        void Serialize(WDataBuffer& outData) override
        {
            outData << (int)Data.Type;
            outData << Data.Color;
            outData << Data.Intensity;
            outData << Data.Radius;
            outData << Data.InnerCone;
            outData << Data.OuterCone;
            outData << Data.Softness;
            outData << Data.AreaWidth;
            outData << Data.AreaHeight;
            outData << Data.AreaTwoSided;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> (int&)Data.Type;
            inData >> Data.Color;
            inData >> Data.Intensity;
            inData >> Data.Radius;
            inData >> Data.InnerCone;
            inData >> Data.OuterCone;
            inData >> Data.Softness;
            inData >> Data.AreaWidth;
            inData >> Data.AreaHeight;
            inData >> Data.AreaTwoSided;
        }

        LightData Data = {};
    };
}