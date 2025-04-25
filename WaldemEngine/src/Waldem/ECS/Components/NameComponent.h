#pragma once
#include "Waldem/ECS/Component.h"

#define MAX_ENTITY_NAME_SIZE 128

namespace Waldem
{
    struct WALDEM_API NameComponent : IComponent<NameComponent>
    {
        WString Name;
        NameComponent() : Name("DefaultEntity") {}
        NameComponent(WString name) : Name(name) {}
        
        void Serialize(WDataBuffer& outData) override 
        {
            Name.Serialize(outData);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            Name.Deserialize(inData);
        }
    };
}
