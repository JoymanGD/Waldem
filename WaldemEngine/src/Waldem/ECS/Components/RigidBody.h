#pragma once

#include "ComponentBase.h"
#include "ColliderComponent.h"

namespace Waldem
{
    struct WALDEM_API RigidBody
    {
        COMPONENT(RigidBody)
            FIELD(bool, IsKinematic)
            FIELD(bool, Gravity)
            FIELD(Vector3, Velocity)
            FIELD(Vector3, AngularVelocity)
            FIELD(float, Mass)
            FIELD(float, LinearDamping)
            FIELD(float, AngularDamping)
            FIELD(Vector3, Force)
            FIELD(Vector3, Torque)
            FIELD(float, Bounciness)
            FIELD(float, Friction)
            FIELD(float, MaxAngularSpeed)
        END_COMPONENT()
        
        bool IsKinematic = false;
        bool Gravity = true;
        Vector3 Velocity = Vector3(0);
        Vector3 AngularVelocity = Vector3(0);
        float Mass = 1.0f;
        float LinearDamping = 0.9f;
        float AngularDamping = 0.9f;
        Vector3 Force = Vector3(0);
        Vector3 Torque = Vector3(0);
        float Bounciness = 0.f;
        float Friction = 0.f;
        float MaxAngularSpeed = 10.f;
        Matrix3 InertiaTensor = Matrix3(0);
        Matrix3 InvInertiaTensor = Matrix3(0);
        float InvMass = 1.0f;

        void ApplyForce(const Vector3& f, const Vector3& deltaPos)
        {
            Force += f;
            Torque += cross(deltaPos, f);
        }

        void Reset()
        {
            Force = Torque = Vector3(0);
        }
    };
}