#pragma once
#include <ecs.h>
#include "Entity.h"
#include "Components/NameComponent.h"
#include "ComponentRegistry.h"

namespace Waldem
{
    class WALDEM_API ECSManager
    {
    public:
        ECSManager()
        {
            ComponentRegistry::Get().RegisterAllComponents();
        }
        
        CEntity* CreateEntity(const String& name)
        {
            CEntity* entity = new CEntity(name);
        
            entity->NativeEntity = NativeECSManager.CreateEntity();
            entity->Add<NameComponent>(name);

            return entity;
        }
        
        void Refresh()
        {
            NativeECSManager.Refresh();
        }
        
        void RegisterEntity(CEntity* entity)
        {
            entity->NativeEntity = NativeECSManager.CreateEntity();

            for (auto component : entity->Components)
            {
                component->RegisterToNativeEntity(entity->NativeEntity);
            }
        }

        template <typename... Ts>
        ecs::EntitiesWith<Ts...> EntitiesWith()
        {
            return NativeECSManager.EntitiesWith<Ts...>();
        }

        template <typename... Ts>
        ecs::EntitiesWithout<Ts...> EntitiesWithout()
        {
            return NativeECSManager.EntitiesWithout<Ts...>();
        }

        ecs::Entities Entities()
        {
            return NativeECSManager.Entities();
        }

    private:
        ecs::Manager NativeECSManager;
    };
}
