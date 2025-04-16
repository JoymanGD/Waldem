#pragma once
#include "Waldem/Serialization/Serializable.h"
#include <ecs.h>
#include "Waldem/Types/String.h"

namespace Waldem
{
    struct WALDEM_API IComponentBase : ISerializable
    {
        virtual void RegisterToNativeEntity(ecs::Entity& entity) = 0;
        virtual WString GetComponentName() = 0;
    };
    
    template<typename T>
    struct WALDEM_API IComponent : IComponentBase
    {
        void Serialize(WDataBuffer& outData) override {}
        void Deserialize(WDataBuffer& inData) override {}
        
        void RegisterToNativeEntity(ecs::Entity& entity) override
        {
            entity.Add<T>((T&)*this);
        }

        WString GetComponentName() override
        {
            WString fullName = typeid(T).name();

            if (fullName.Find("struct ") == 0)
                fullName = fullName.Substr(7);
            else if (fullName.Find("class ") == 0)
                fullName = fullName.Substr(6);

            size_t pos = fullName.Find("::");
            if (pos != std::string::npos)
                fullName = fullName.Substr(pos + 2);

            return fullName;
        }
    };
}
