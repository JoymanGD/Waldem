#pragma once

#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API PostProcessComponent
    {        
        FIELD()
        float BrightThreshold = .9999f;
        FIELD()
        float BloomIntensity = 19.f;
        FIELD()
        float Exposure = 0.3f;
        FIELD()
        float Saturation = 1.3f;
        
        PostProcessComponent() {}
    };
}
#include "PostProcessComponent.generated.h"
