#include "wdpch.h"
#include "RigidBodyBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/RigidBody.h"

namespace Waldem::Bindings
{
    namespace
    {
        struct ScriptVector3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

        void RigidBody_GetVelocity(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) { *out = {}; return; }
            const auto& rb = entity.get<RigidBody>();
            out->x = rb.Velocity.x; out->y = rb.Velocity.y; out->z = rb.Velocity.z;
        }

        void RigidBody_SetVelocity(uint64_t entityId, ScriptVector3* velocity)
        {
            if(velocity == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return;
            auto& rb = entity.get_mut<RigidBody>();
            rb.Velocity = Vector3(velocity->x, velocity->y, velocity->z);
            entity.modified<RigidBody>();
        }

        void RigidBody_GetAngularVelocity(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) { *out = {}; return; }
            const auto& rb = entity.get<RigidBody>();
            out->x = rb.AngularVelocity.x; out->y = rb.AngularVelocity.y; out->z = rb.AngularVelocity.z;
        }

        void RigidBody_SetAngularVelocity(uint64_t entityId, ScriptVector3* angularVelocity)
        {
            if(angularVelocity == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return;
            auto& rb = entity.get_mut<RigidBody>();
            rb.AngularVelocity = Vector3(angularVelocity->x, angularVelocity->y, angularVelocity->z);
            entity.modified<RigidBody>();
        }

        void RigidBody_AddForce(uint64_t entityId, ScriptVector3* force)
        {
            if(force == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return;
            auto& rb = entity.get_mut<RigidBody>();
            rb.Force += Vector3(force->x, force->y, force->z);
            entity.modified<RigidBody>();
        }

        bool RigidBody_GetIsGrounded(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return false;
            return entity.get<RigidBody>().IsGrounded;
        }

        float RigidBody_GetMass(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return 0.0f;
            return entity.get<RigidBody>().Mass;
        }

        void RigidBody_SetMass(uint64_t entityId, float mass)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return;
            entity.get_mut<RigidBody>().Mass = mass;
            entity.modified<RigidBody>();
        }

        bool RigidBody_GetIsKinematic(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return false;
            return entity.get<RigidBody>().IsKinematic;
        }

        void RigidBody_SetIsKinematic(uint64_t entityId, bool isKinematic)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<RigidBody>()) return;
            entity.get_mut<RigidBody>().IsKinematic = isKinematic;
            entity.modified<RigidBody>();
        }
    }

    void RegisterRigidBodyCalls(Mono* runtime)
    {
        BIND(runtime, RigidBody_GetVelocity);
        BIND(runtime, RigidBody_SetVelocity);
        BIND(runtime, RigidBody_GetAngularVelocity);
        BIND(runtime, RigidBody_SetAngularVelocity);
        BIND(runtime, RigidBody_AddForce);
        BIND(runtime, RigidBody_GetIsGrounded);
        BIND(runtime, RigidBody_GetMass);
        BIND(runtime, RigidBody_SetMass);
        BIND(runtime, RigidBody_GetIsKinematic);
        BIND(runtime, RigidBody_SetIsKinematic);
    }
}
