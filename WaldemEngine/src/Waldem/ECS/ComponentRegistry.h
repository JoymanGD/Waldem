#pragma once
#include "Component.h"
#include <unordered_map>
#include <functional>

#include "Waldem/Types/WMap.h"

namespace Waldem
{
    using ComponentCreateFn = std::function<IComponentBase*()>; // creates a new empty component
    using ComponentFromEntityFn = std::function<IComponentBase*(ecs::Entity&)>; // fetches from native entity
    using ComponentHasFn = std::function<bool(const ecs::Entity&)>; // fetches from native entity

    struct ComponentEntry
    {
        ComponentCreateFn createFn;
        ComponentFromEntityFn getFromEntityFn;
        ComponentHasFn hasFn;
    };

    class WALDEM_API ComponentRegistry
    {
    public:
        WMap<WString, ComponentEntry> Entries;

        static ComponentRegistry& Get()
        {
            static ComponentRegistry instance;
            return instance;
        }

        void RegisterAllComponents();

        template <typename T>
        void RegisterComponent(const std::string& typeName)
        {
            Entries[typeName] = ComponentEntry{
                []() -> IComponentBase* {
                    return new T(); // requires T to inherit from IComponent<T>
                },
                [](ecs::Entity& entity) -> IComponentBase* {
                    return &entity.Get<T>(); // assumes component is already in entity
                },
                [](const ecs::Entity& entity) -> bool {
                    return entity.Has<T>();
                }
            };
        }

        IComponentBase* CreateComponent(const WString& typeName)
        {
            auto it = Entries.Find(typeName);
            if (it)
                return it->createFn();
            return nullptr;
        }

        IComponentBase* GetComponentFromEntity(const WString& typeName, ecs::Entity& entity)
        {
            auto it = Entries.Find(typeName);
            if (it)
                return it->getFromEntityFn(entity);
            return nullptr;
        }
    };

    // Auto-registers component at startup
    #define REGISTER_COMPONENT(Type) \
    static bool _##Type##_registered = []() { \
    ComponentRegistry::Get().RegisterComponent<Type>(#Type); \
    return true; \
    }()
}
