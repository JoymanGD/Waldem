#pragma once

#include "flecs.h"
#include "Components/Selected.h"
#include "Components\SceneEntity.h"
#include "Waldem/Types/FreeList.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    namespace ECS
    {
        inline flecs::world World;
        inline WMap<WString, flecs::entity> RegisteredComponents;
        inline FreeList HierarchySlots;
        
        inline int GetEntitiesCount() { return World.query<SceneEntity>().count(); }

        inline bool NameExists(const WString& name)
        {
            return World.lookup(name) != flecs::entity::null();
        }

        inline bool NameExists(const std::string& name)
        {
            return World.lookup(name.c_str()) != flecs::entity::null();
        }

        inline void FormatName(WString& name)
        {
            while(NameExists(name))
            {
                name += "_1";
            }
        }

        inline void FormatName(std::string& name)
        {
            while(NameExists(name))
            {
                name += "_1";
            }
        }
        
        flecs::entity CreateEntity(const WString& name = "", bool enabled = true);
        flecs::entity CreateSceneEntity(const WString& name = "", bool enabled = true, bool visibleInHierarchy = true);
        flecs::entity CloneSceneEntity(flecs::entity entity);
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
