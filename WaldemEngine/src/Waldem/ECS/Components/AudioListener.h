#pragma once
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API AudioListener : IComponent<AudioListener>
    {
        void Serialize(WDataBuffer& outData) override {}
        void Deserialize(WDataBuffer& inData) override {}
    };
}
