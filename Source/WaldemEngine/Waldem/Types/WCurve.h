#pragma once

namespace Waldem
{
    struct WCurve
    {
        static constexpr uint MaxPoints = 5;
        
        Vector2 Points[MaxPoints] =
        {
            {0.0f, 0.0f},
            {1.0f, 1.0f}
        };
        
        Vector2 Min = { 0, 0 };
        Vector2 Max = { 1, 1 };
        uint Count = 2;
        uint Padding[3];
    };
}
