#pragma once
#include "Waldem/Serialization/Serializable.h"
#include "Waldem/ECS/ComponentRegistry.h"
#include "Component.h"
#include <ecs.h>

#include "Waldem/Types/String.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    class WALDEM_API CEntity : ISerializable
    {
    public:
        // WArray<IComponentBase*> Components;
        std::vector<IComponentBase*> Components;
        ecs::Entity NativeEntity;
        
        CEntity(const WString& name = "NewEntity") : Name(name) {}
        
        bool operator==(const CEntity& e) const
        {
            return NativeEntity == e.NativeEntity;
        }

        bool operator!=(const CEntity& e) const
        {
            return !(*this == e);
        }
        
        template <typename T, typename... Ts>
        T& Add(Ts&&... constructor_args)
        {
            auto* component = new T(std::forward<Ts>(constructor_args)...);
            Components.push_back(component);
            NativeEntity.Add<T>(*component);
            return *component;
        }

        template <typename... Ts>
        void Remove()
        {
            auto& component = NativeEntity.Get<Ts...>();
            Components.erase(component);
        
            NativeEntity.Remove<Ts...>();
        }

        template <typename... Ts>
        bool Has() const
        {
            return NativeEntity.Has<Ts...>();
        }

        template <typename... Ts>
        bool HasAny() const
        {
            return NativeEntity.HasAny<Ts...>();
        }

        template <typename... Ts>
        decltype(auto) Get() const
        {
            return NativeEntity.Get<Ts...>();
        }

        template <typename... Ts>
        decltype(auto) Get()
        {
            return NativeEntity.Get<Ts...>();
        }

        void Clear()
        {
            NativeEntity.Clear();
            Components.clear();
        }

        bool IsAlive() const
        {
            return NativeEntity.IsAlive();
        }

        void Destroy()
        {
            Components.clear();
            NativeEntity.Destroy();
        }

        WString GetName() const { return Name; }
        
        void Serialize(WDataBuffer& outData) override
        {
            Name.Serialize(outData);

            auto& registry = ComponentRegistry::Get();
            uint count = 0;

            // First count how many components exist in NativeEntity
            for (const auto& [typeName, entry] : registry.Entries)
            {
                if (entry.hasFn(NativeEntity))
                {
                    auto name = typeName;
                    count++;
                }
            }

            outData << count;

            // Serialize each present component
            for (const auto& [typeName, entry] : registry.Entries)
            {
                if (entry.hasFn(NativeEntity))
                {
                    const_cast<WString&>(typeName).Serialize(outData); // WString
                    IComponentBase* comp = entry.getFromEntityFn(NativeEntity);
                    comp->Serialize(outData);
                }
            }
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            Name.Deserialize(inData);

            uint count = 0;
            inData >> count;

            Components.clear();
            Components.resize(count);

            for (size_t i = 0; i < count; ++i)
            {
                WString typeName = "";
                typeName.Deserialize(inData);

                IComponentBase* comp = ComponentRegistry::Get().CreateComponent(typeName);
                if (comp)
                {
                    comp->Deserialize(inData);
                    Components[i] = comp;
                }
            }
        }

    private:
        WString Name;
    };
}