#pragma once
#include "Waldem/ECS/Component.h"

#define MAX_ENTITY_NAME_SIZE 128

namespace Waldem
{
    struct WALDEM_API NameComponent : IComponent<NameComponent>
    {
        String Name;
        NameComponent() : Name("DefaultEntity") {}
        NameComponent(String name) : Name(name) {}
        
        void Serialize(WDataBuffer& outData) override
        {
            outData << Name;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Name;
        }
    };
}
