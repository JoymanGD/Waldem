#include "wdpch.h"
#include "EntityBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/Camera.h"
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
            Light = 4
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
            default: return false;
            }
        }
    }

    void RegisterEntityCalls(Mono* runtime)
    {
        BIND(runtime, Entity_HasComponent);
    }
}
