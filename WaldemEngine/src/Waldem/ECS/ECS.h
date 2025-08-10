#pragma once

#include "flecs.h"
#include "Components\SceneEntity.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    namespace ECS
    {
        inline flecs::world World;
        inline WMap<WString, flecs::entity> RegisteredComponents;
        
        inline int GetEntitiesCount() { return World.query<SceneEntity>().count(); }

        inline flecs::entity CreateEntity(const WString& name = "", bool enabled = true)
        {
            flecs::entity entity = World.entity(name);

            if(enabled)
            {
                entity.enable();
            }
            else
            {
                entity.disable();
            }

            return entity;
        }

        inline flecs::entity CreateSceneEntity(const WString& name = "", bool enabled = true, bool visibleInHierarchy = true)
        {
            auto count = GetEntitiesCount();
            flecs::entity entity = World.entity(name).set<SceneEntity>({
                .ParentId = 0,
                .HierarchySlot = (float)count,
                .VisibleInHierarchy = visibleInHierarchy,
            });

            if(enabled)
            {
                entity.enable();
            }
            else
            {
                entity.disable();
            }

            return entity;
        }
        
        void RegisterTypes();
        void RegisterComponents();

        class Core
        {
        public:
            void Initialize()
            {
                World = flecs::world();
                RegisterTypes();
                RegisterComponents();
            }
        };
    }
}
