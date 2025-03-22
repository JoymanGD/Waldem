#pragma once

namespace Waldem
{
    struct WALDEM_API BloomPostProcess
    {
        BloomPostProcess(float brightThreshold = 0.8f, float bloomIntensity = 2.f, Vector2 texelSize = Vector2(0.001f))
            : BrightThreshold(brightThreshold), BloomIntensity(bloomIntensity), TexelSize(texelSize) {}
        
        float BrightThreshold = 0.8f;
        float BloomIntensity = 2.f;
        Vector2 TexelSize = Vector2(0.001f);
    };
}
