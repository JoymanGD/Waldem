#include "wdpch.h"
#include "QuaternionBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/Types/MathTypes.h"

namespace Waldem::Bindings
{
    namespace
    {
        struct ScriptVector3
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        struct ScriptQuaternion
        {
            float w = 1.0f;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        Quaternion ToNativeQuaternion(const ScriptQuaternion& quaternion)
        {
            return normalize(Quaternion(quaternion.w, quaternion.x, quaternion.y, quaternion.z));
        }

        Vector3 ToNativeVector3(const ScriptVector3& vector)
        {
            return Vector3(vector.x, vector.y, vector.z);
        }

        void WriteQuaternion(const Quaternion& quaternion, ScriptQuaternion* out)
        {
            if(out == nullptr)
            {
                return;
            }

            const Quaternion normalized = normalize(quaternion);
            out->w = normalized.w;
            out->x = normalized.x;
            out->y = normalized.y;
            out->z = normalized.z;
        }

        void WriteVector3(const Vector3& vector, ScriptVector3* out)
        {
            if(out == nullptr)
            {
                return;
            }

            out->x = vector.x;
            out->y = vector.y;
            out->z = vector.z;
        }

        void Quaternion_Normalize(ScriptQuaternion* value, ScriptQuaternion* out)
        {
            if(value == nullptr || out == nullptr)
            {
                return;
            }

            Quaternion quaternion = Quaternion(value->w, value->x, value->y, value->z);
            if(length(quaternion) < 1e-6f)
            {
                quaternion = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
            }

            WriteQuaternion(normalize(quaternion), out);
        }

        void Quaternion_Multiply(ScriptQuaternion* left, ScriptQuaternion* right, ScriptQuaternion* out)
        {
            if(left == nullptr || right == nullptr || out == nullptr)
            {
                return;
            }

            WriteQuaternion(ToNativeQuaternion(*left) * ToNativeQuaternion(*right), out);
        }

        void Quaternion_RotateVector(ScriptQuaternion* rotation, ScriptVector3* point, ScriptVector3* out)
        {
            if(rotation == nullptr || point == nullptr || out == nullptr)
            {
                return;
            }

            WriteVector3(ToNativeQuaternion(*rotation) * ToNativeVector3(*point), out);
        }

        void Quaternion_Euler(ScriptVector3* eulerDegrees, ScriptQuaternion* out)
        {
            if(eulerDegrees == nullptr || out == nullptr)
            {
                return;
            }

            WriteQuaternion(Quaternion(glm::radians(ToNativeVector3(*eulerDegrees))), out);
        }

        void Quaternion_AngleAxis(float angleDegrees, ScriptVector3* axis, ScriptQuaternion* out)
        {
            if(axis == nullptr || out == nullptr)
            {
                return;
            }

            Vector3 nativeAxis = ToNativeVector3(*axis);
            if(length(nativeAxis) < 1e-6f)
            {
                WriteQuaternion(Quaternion(1.0f, 0.0f, 0.0f, 0.0f), out);
                return;
            }

            WriteQuaternion(angleAxis(glm::radians(angleDegrees), normalize(nativeAxis)), out);
        }

        void Quaternion_LookRotation(ScriptVector3* forward, ScriptVector3* up, ScriptQuaternion* out)
        {
            if(forward == nullptr || up == nullptr || out == nullptr)
            {
                return;
            }

            Vector3 nativeForward = ToNativeVector3(*forward);
            Vector3 nativeUp = ToNativeVector3(*up);

            if(length(nativeForward) < 1e-6f)
            {
                WriteQuaternion(Quaternion(1.0f, 0.0f, 0.0f, 0.0f), out);
                return;
            }

            nativeForward = normalize(nativeForward);
            if(length(nativeUp) < 1e-6f)
            {
                nativeUp = Vector3(0, 1, 0);
            }
            else
            {
                nativeUp = normalize(nativeUp);
            }

            if(glm::abs(dot(nativeForward, nativeUp)) > 0.99f)
            {
                nativeUp = glm::abs(dot(nativeForward, Vector3(0, 1, 0))) > 0.99f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
            }

            WriteQuaternion(quatLookAt(nativeForward, nativeUp), out);
        }

        void Quaternion_ToEuler(ScriptQuaternion* value, ScriptVector3* out)
        {
            if(value == nullptr || out == nullptr)
            {
                return;
            }

            WriteVector3(degrees(eulerAngles(ToNativeQuaternion(*value))), out);
        }
    }

    void RegisterQuaternionCalls(Mono* runtime)
    {
        BIND(runtime, Quaternion_Normalize);
        BIND(runtime, Quaternion_Multiply);
        BIND(runtime, Quaternion_RotateVector);
        BIND(runtime, Quaternion_Euler);
        BIND(runtime, Quaternion_AngleAxis);
        BIND(runtime, Quaternion_LookRotation);
        BIND(runtime, Quaternion_ToEuler);
    }
}
