#pragma once

namespace Waldem
{
    struct WGradient
    {
        static constexpr uint MaxPoints = 5;
        
        Vector4 Points[MaxPoints] =
        {
            {1.0f, 1.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        };

        uint Count = 2;
        uint Padding[3] = {};
    };
}
