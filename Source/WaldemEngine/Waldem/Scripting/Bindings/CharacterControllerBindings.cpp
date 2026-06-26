#include "wdpch.h"
#include "CharacterControllerBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/CharacterController.h"

namespace Waldem::Bindings
{
    namespace
    {
        struct ScriptVector3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

        void CharacterController_GetMoveVelocity(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<CharacterController>()) { *out = {}; return; }
            const auto& controller = entity.get<CharacterController>();
            out->x = controller.MoveVelocity.x;
            out->y = controller.MoveVelocity.y;
            out->z = controller.MoveVelocity.z;
        }

        void CharacterController_SetMoveVelocity(uint64_t entityId, ScriptVector3* velocity)
        {
            if(velocity == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<CharacterController>()) return;
            auto& controller = entity.get_mut<CharacterController>();
            controller.MoveVelocity = Vector3(velocity->x, velocity->y, velocity->z);
            entity.modified<CharacterController>();
        }

        bool CharacterController_GetIsGrounded(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<CharacterController>()) return false;
            return entity.get<CharacterController>().IsGrounded;
        }

        float CharacterController_GetJumpSpeed(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<CharacterController>()) return 0.0f;
            return entity.get<CharacterController>().JumpSpeed;
        }

        void CharacterController_SetJumpSpeed(uint64_t entityId, float jumpSpeed)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<CharacterController>()) return;
            auto& controller = entity.get_mut<CharacterController>();
            controller.JumpSpeed = jumpSpeed;
            entity.modified<CharacterController>();
        }

        void CharacterController_Jump(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<CharacterController>()) return;
            auto& controller = entity.get_mut<CharacterController>();
            controller.JumpRequested = true;
            entity.modified<CharacterController>();
        }
    }

    void RegisterCharacterControllerCalls(Mono* runtime)
    {
        BIND(runtime, CharacterController_GetMoveVelocity);
        BIND(runtime, CharacterController_SetMoveVelocity);
        BIND(runtime, CharacterController_GetIsGrounded);
        BIND(runtime, CharacterController_GetJumpSpeed);
        BIND(runtime, CharacterController_SetJumpSpeed);
        BIND(runtime, CharacterController_Jump);
    }
}
