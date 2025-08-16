#pragma once
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    struct LineData
    {
        Vector3 Start;
        Vector3 End;
        Vector3 Color;
    };
    
    struct WALDEM_API LineComponent
    {
        WArray<LineData> Lines;

        LineComponent() = default;
        
        LineComponent(WArray<LineData> lines) : Lines(lines) {}
    };
}
