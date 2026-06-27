#include "wdpch.h"
#include "TransformBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/ECS.h"

namespace Waldem::Bindings
{
    namespace
    {
        struct ScriptVector3 { float x = 0.0f, y = 0.0f, z = 0.0f; };
        struct ScriptQuaternion { float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f; };

        void Transform_GetPosition(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = {}; return; }
            const auto& t = entity.get<Transform>();
            out->x = t.Position.x; out->y = t.Position.y; out->z = t.Position.z;
        }

        void Transform_SetPosition(uint64_t entityId, ScriptVector3* position)
        {
            if(position == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.SetPosition(position->x, position->y, position->z);
            entity.modified<Transform>();
        }

        void Transform_GetForward(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = {}; return; }
            auto v = entity.get<Transform>().GetForwardVector();
            out->x = v.x; out->y = v.y; out->z = v.z;
        }

        void Transform_SetForward(uint64_t entityId, Vector3 forward)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { return; }
            entity.get_mut<Transform>().SetForwardVector(forward);
        }

        void Transform_GetRight(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = {}; return; }
            auto v = entity.get<Transform>().GetRightVector();
            out->x = v.x; out->y = v.y; out->z = v.z;
        }

        void Transform_GetUp(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = {}; return; }
            auto v = entity.get<Transform>().GetUpVector();
            out->x = v.x; out->y = v.y; out->z = v.z;
        }

        void Transform_Translate(uint64_t entityId, ScriptVector3* translation)
        {
            if(translation == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.Translate(Vector3(translation->x, translation->y, translation->z));
            entity.modified<Transform>();
        }

        void Transform_GetRotation(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = {}; return; }
            const auto& t = entity.get<Transform>();
            out->x = t.Rotation.x; out->y = t.Rotation.y; out->z = t.Rotation.z;
        }

        void Transform_SetRotation(uint64_t entityId, ScriptVector3* rotation)
        {
            if(rotation == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.SetRotation(rotation->x, rotation->y, rotation->z);
            entity.modified<Transform>();
        }

        void Transform_GetLocalRotation(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>())
            {
                *out = {};
                return;
            }

            const Vector3 localRotation = ECS::GetLocalRotation(entity);
            out->x = localRotation.x;
            out->y = localRotation.y;
            out->z = localRotation.z;
        }

        void Transform_SetLocalRotation(uint64_t entityId, ScriptVector3* rotation)
        {
            if(rotation == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            ECS::SetLocalRotation(entity, Vector3(rotation->x, rotation->y, rotation->z));
        }

        void Transform_Rotate(uint64_t entityId, ScriptVector3* rotationDelta)
        {
            if(rotationDelta == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.Rotate(rotationDelta->y, rotationDelta->x, rotationDelta->z);
            entity.modified<Transform>();
        }

        void Transform_GetRotationQuaternion(uint64_t entityId, ScriptQuaternion* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = { 1.0f, 0.0f, 0.0f, 0.0f }; return; }
            const auto& rotation = entity.get<Transform>().RotationQuat;
            out->w = rotation.w;
            out->x = rotation.x;
            out->y = rotation.y;
            out->z = rotation.z;
        }

        void Transform_SetRotationQuaternion(uint64_t entityId, ScriptQuaternion* rotation)
        {
            if(rotation == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.SetRotation(Quaternion(rotation->w, rotation->x, rotation->y, rotation->z));
            entity.modified<Transform>();
        }

        void Transform_RotateQuaternion(uint64_t entityId, ScriptQuaternion* rotationDelta)
        {
            if(rotationDelta == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.Rotate(Quaternion(rotationDelta->w, rotationDelta->x, rotationDelta->y, rotationDelta->z));
            entity.modified<Transform>();
        }

        void Transform_GetScale(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) { *out = {}; return; }
            const auto& t = entity.get<Transform>();
            out->x = t.LocalScale.x; out->y = t.LocalScale.y; out->z = t.LocalScale.z;
        }

        void Transform_SetScale(uint64_t entityId, ScriptVector3* scale)
        {
            if(scale == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.SetScale(scale->x, scale->y, scale->z);
            entity.modified<Transform>();
        }

        void Transform_LookAt(uint64_t entityId, ScriptVector3* target)
        {
            if(target == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.LookAt(Vector3(target->x, target->y, target->z));
            entity.modified<Transform>();
        }

        void Transform_LookAtWithUp(uint64_t entityId, ScriptVector3* target, ScriptVector3* up)
        {
            if(target == nullptr || up == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.LookAt(Vector3(target->x, target->y, target->z), Vector3(up->x, up->y, up->z));
            entity.modified<Transform>();
        }

        void Transform_RotateAround(uint64_t entityId, ScriptVector3* point, ScriptVector3* axis, float angleDegrees)
        {
            if(point == nullptr || axis == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Transform>()) return;
            auto& t = entity.get_mut<Transform>();
            t.RotateAround(Vector3(point->x, point->y, point->z), Vector3(axis->x, axis->y, axis->z), angleDegrees);
            entity.modified<Transform>();
        }
    }

    void RegisterTransformCalls(Mono* runtime)
    {
        BIND(runtime, Transform_GetPosition);
        BIND(runtime, Transform_SetPosition);
        BIND(runtime, Transform_GetForward);
        BIND(runtime, Transform_SetForward);
        BIND(runtime, Transform_GetRight);
        BIND(runtime, Transform_GetUp);
        BIND(runtime, Transform_Translate);
        BIND(runtime, Transform_GetRotation);
        BIND(runtime, Transform_SetRotation);
        BIND(runtime, Transform_GetLocalRotation);
        BIND(runtime, Transform_SetLocalRotation);
        BIND(runtime, Transform_Rotate);
        BIND(runtime, Transform_GetRotationQuaternion);
        BIND(runtime, Transform_SetRotationQuaternion);
        BIND(runtime, Transform_RotateQuaternion);
        BIND(runtime, Transform_GetScale);
        BIND(runtime, Transform_SetScale);
        BIND(runtime, Transform_LookAt);
        BIND(runtime, Transform_LookAtWithUp);
        BIND(runtime, Transform_RotateAround);
    }
}
