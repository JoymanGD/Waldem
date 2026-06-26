#include "wdpch.h"
#include "EntityBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/AnimatorComponent.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/CharacterController.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem::Bindings
{
    namespace
    {
        enum class ScriptComponentKind : int32_t
        {
            None = 0,
            Transform = 1,
            Camera = 2,
            RigidBody = 3,
            Light = 4,
            Animator = 5,
            CharacterController = 6
        };

        bool Entity_HasComponent(uint64_t entityId, int32_t componentKind)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive())
                return false;

            switch(static_cast<ScriptComponentKind>(componentKind))
            {
            case ScriptComponentKind::Transform: return entity.has<Transform>();
            case ScriptComponentKind::Camera:    return entity.has<Camera>();
            case ScriptComponentKind::RigidBody: return entity.has<RigidBody>();
            case ScriptComponentKind::Light:     return entity.has<Light>();
            case ScriptComponentKind::Animator:  return entity.has<AnimatorComponent>();
            case ScriptComponentKind::CharacterController: return entity.has<CharacterController>();
            default: return false;
            }
        }

        void Entity_AddComponent(uint64_t entityId, int32_t componentKind)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive())
                return;

            switch(static_cast<ScriptComponentKind>(componentKind))
            {
                case ScriptComponentKind::Transform:
                {
                    entity.add<Transform>();
                    break;
                }
                case ScriptComponentKind::Camera:
                {
                    entity.add<Camera>();
                    break;
                }
                case ScriptComponentKind::RigidBody:
                {
                    entity.add<RigidBody>();
                    break;
                }
                case ScriptComponentKind::Light:
                {
                    entity.add<Light>();
                    break;
                }
                case ScriptComponentKind::Animator:
                {
                    entity.add<AnimatorComponent>();
                    break;
                }
                case ScriptComponentKind::CharacterController:
                {
                    entity.add<CharacterController>();
                    break;
                }
            }
        }

        void Entity_Destroy(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);

            if(entity.is_alive())
            {
                entity.destruct();
            }
        }
    }

    void RegisterEntityCalls(Mono* runtime)
    {
        BIND(runtime, Entity_HasComponent);
        BIND(runtime, Entity_AddComponent);
        BIND(runtime, Entity_Destroy);
    }
}
