#pragma once

namespace Waldem
{
    struct WColor
    {
        float r;
        float g;
        float b;
        float a;

        WColor() = default;

        WColor(float r, float b, float g, float a) : r(r), g(g), b(b), a(a) {}
    };
}
