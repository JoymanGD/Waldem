#pragma once
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API Selected : IComponent<Selected>
    {
        void Serialize(WDataBuffer& outData) override {}
        void Deserialize(WDataBuffer& inData) override {}
    };
}
