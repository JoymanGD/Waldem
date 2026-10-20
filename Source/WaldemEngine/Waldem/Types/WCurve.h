#pragma once
#include "WArray.h"

namespace Waldem
{
    struct WCurve
    {
        WArray<Vector2> Points =
        {
            {0.0f, 0.0f},
            {1.0f, 1.0f}
        };

        Vector2 Min = {0.0f, 0.0f};
        Vector2 Max = {1.0f, 1.0f};
    };
}
