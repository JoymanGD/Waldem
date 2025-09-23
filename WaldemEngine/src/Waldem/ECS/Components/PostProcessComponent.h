#pragma once

#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    struct WALDEM_API PostProcessComponent
    {
        COMPONENT(PostProcessComponent)
            FIELD(float, BrightThreshold)
            FIELD(float, BloomIntensity)
            FIELD(float, Exposure)
            FIELD(float, Saturation)
        END_COMPONENT()
        
        float BrightThreshold = .9999f;
        float BloomIntensity = 19.f;
        float Exposure = 0.3f;
        float Saturation = 1.3f;
    };
}
