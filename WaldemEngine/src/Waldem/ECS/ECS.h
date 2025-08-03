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
        inline uint64 SceneEntitiesCount = 0;
        inline WMap<WString, flecs::entity> RegisteredComponents;

        inline flecs::entity CreateEntity(const WString& name, bool enabled = true, bool visibleInHierarchy = true)
        {
            flecs::entity entity = World.entity(name).set<SceneEntity>({
                .ParentId = 0,
                .HierarchySlot = (float)SceneEntitiesCount++,
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
