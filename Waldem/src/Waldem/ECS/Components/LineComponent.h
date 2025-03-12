#pragma once
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    struct LineComponent
    {
        LineComponent(WArray<LineData> lines) : Lines(lines) {}

        WArray<LineData> Lines;
    };
}
