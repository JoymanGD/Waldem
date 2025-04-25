#pragma once
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct LineData
    {
        Vector3 Start;
        Vector3 End;
        Vector3 Color;
    };
    
    struct WALDEM_API LineComponent : IComponent<LineComponent>
    {
        WArray<LineData> Lines;

        LineComponent() = default;
        
        LineComponent(WArray<LineData> lines) : Lines(lines) {}
        
        void Serialize(WDataBuffer& outData) override
        {
            Lines.Serialize(outData);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            Lines.Deserialize(inData);
        }
    };
}
