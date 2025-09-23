#pragma once

namespace Waldem
{
    struct WALDEM_API BloomPostProcess
    {
        COMPONENT(BloomPostProcess)
            FIELD(float, BrightThreshold)
            FIELD(float, BloomIntensity)
        END_COMPONENT()
        
        float BrightThreshold = 0.8f;
        float BloomIntensity = 2.f;
    };
}
