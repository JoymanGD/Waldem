#pragma once
#include "ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API AudioListener
    {
        FIELD()
        float Volume = 1.0f;
        
        AudioListener() {}
    };
}
#include "AudioListener.generated.h"
