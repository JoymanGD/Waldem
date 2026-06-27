#pragma once

#include "../../../../Vendor/flecs/include/flecs.h"
#include "Components/SceneEntity.h"
#include "Systems/CoreSystem.h"
#include "Waldem/Types/FreeList.h"
#include "Waldem/Types/MathTypes.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    namespace ECS
    {
        struct OnFixedUpdate {};
        struct OnLateUpdate {};
        struct OnDraw {};
        struct OnGUI {};
        
        using Entity = flecs::entity;
        using EntityT = flecs::entity_t;
        using Id = flecs::id;
        using IdT = flecs::id_t;
        using Iter = flecs::iter;
        using TypeSerializer = flecs::TypeSerializer;
        using MetaOp = flecs::meta::op_t;
        using ComponentRegisterFn = void(*)(flecs::world&);

        inline const EntityT OnAdd = flecs::OnAdd;
        inline const EntityT OnSet = flecs::OnSet;
        inline const EntityT OnRemove = flecs::OnRemove;
        inline const EntityT OnUpdate = flecs::OnUpdate;
        
        extern WALDEM_API flecs::world World;
        extern WALDEM_API EntityT UpdatePipeline;
        extern WALDEM_API EntityT FixedUpdatePipeline;
        extern WALDEM_API EntityT LateUpdatePipeline;
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

        inline void RunLateUpdatePipeline(float deltaTime)
        {
            World.run_pipeline(LateUpdatePipeline, deltaTime);
        }

        inline void RunDrawPipeline(float deltaTime)
        {
            World.run_pipeline(DrawPipeline, deltaTime);
        }

        inline void RunGUIPipeline(float deltaTime)
        {
            World.run_pipeline(GUIPipeline, deltaTime);
        }

        void WALDEM_API UpdateRenderTransforms(float interpolationAlpha);
        
        inline int GetEntitiesCount() { return World.query<SceneEntity>().count(); }

        inline const ecs_type_info_t* GetTypeInfo(IdT componentId)
        {
            return ecs_get_type_info(World.c_ptr(), componentId);
        }

        inline void* ValueNew(IdT componentId)
        {
            return ecs_value_new(World.c_ptr(), componentId);
        }

        inline void ValueCopy(IdT componentId, void* dst, const void* src)
        {
            ecs_value_copy(World.c_ptr(), componentId, dst, src);
        }

        inline void ValueFree(IdT componentId, void* data)
        {
            ecs_value_free(World.c_ptr(), componentId, data);
        }
        
        Entity WALDEM_API CreateEntity(const WString& name = "", bool enabled = true);
        Entity WALDEM_API CreateSceneEntity(const WString& name, bool enabled = true, bool visibleInHierarchy = true);
        Entity WALDEM_API CloneSceneEntity(Entity entity);
        void WALDEM_API SetParent(Entity child, Entity parent, bool keepWorldTransform = true);
        void WALDEM_API ClearParent(Entity child, bool keepWorldTransform = true);
        Entity WALDEM_API GetParent(Entity child);
        Vector3 WALDEM_API GetLocalRotation(Entity entity);
        void WALDEM_API SetLocalRotation(Entity entity, const Vector3& rotation);
        void WALDEM_API RebuildParentRelations();
        void WALDEM_API Shutdown();

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
