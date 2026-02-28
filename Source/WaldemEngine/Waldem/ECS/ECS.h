#pragma once

#include "flecs.h"
#include "Components/SceneEntity.h"
#include "Systems/CoreSystem.h"
#include "Waldem/Types/FreeList.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    namespace ECS
    {
        struct OnFixedUpdate {};
        struct OnDraw {};
        struct OnGUI {};
        
        using Entity = flecs::entity;
        using EntityT = flecs::entity_t;
        using Id = flecs::id;
        using ComponentRegisterFn = void(*)(flecs::world&);
        
        extern WALDEM_API flecs::world World;
        extern WALDEM_API EntityT UpdatePipeline;
        extern WALDEM_API EntityT FixedUpdatePipeline;
        extern WALDEM_API EntityT DrawPipeline; 
        extern WALDEM_API EntityT GUIPipeline;
        extern WALDEM_API WMap<WString, Entity> RegisteredComponents;
        extern WALDEM_API FreeList HierarchySlots;

        inline void RunUpdatePipeline(float deltaTime)
        {
            World.run_pipeline(UpdatePipeline, deltaTime);
        }

        inline void RunFixedUpdatePipeline(float fixedDeltaTime)
        {
            World.run_pipeline(FixedUpdatePipeline, fixedDeltaTime);
        }

        inline void RunDrawPipeline(float deltaTime)
        {
            World.run_pipeline(DrawPipeline, deltaTime);
        }

        inline void RunGUIPipeline(float deltaTime)
        {
            World.run_pipeline(GUIPipeline, deltaTime);
        }
        
        inline int GetEntitiesCount() { return World.query<SceneEntity>().count(); }
        
        Entity WALDEM_API CreateEntity(const WString& name = "", bool enabled = true);
        Entity WALDEM_API CreateSceneEntity(const WString& name, bool enabled = true, bool visibleInHierarchy = true);
        Entity WALDEM_API CloneSceneEntity(Entity entity);
        void WALDEM_API SetParent(Entity child, Entity parent, bool keepWorldTransform = true);
        void WALDEM_API ClearParent(Entity child, bool keepWorldTransform = true);
        Entity WALDEM_API GetParent(Entity child);
        void WALDEM_API RebuildParentRelations();

        class Core
        {
        public:
            static inline WArray<ComponentRegisterFn> ComponentsRegistry;
            
            void Initialize();
            void InitializeSystems();
            
        private:
            WArray<ICoreSystem*> Systems;
            
            void RegisterTypes();
            void RegisterAllComponents();
        };
        
        inline void RegisterComponent(ComponentRegisterFn fn)
        {
            Core::ComponentsRegistry.Add(fn);
        }
    }
}
