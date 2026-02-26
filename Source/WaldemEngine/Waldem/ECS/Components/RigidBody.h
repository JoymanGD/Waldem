#pragma once

#include "ComponentBase.h"
#include "ColliderComponent.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API RigidBody
    {        
        FIELD()
        bool IsKinematic = false;
        FIELD()
        bool HasGravity = true;
        FIELD()
        float MaxSlope = 30;
        FIELD()
        Vector3 Velocity = Vector3(0);
        FIELD()
        Vector3 AngularVelocity = Vector3(0);
        FIELD()
        float Mass = 1.0f;
        FIELD()
        float LinearDamping = 0.05f;
        FIELD()
        float AngularDamping = 0.9f;
        FIELD()
        Vector3 Force = Vector3(0);
        FIELD()
        Vector3 Torque = Vector3(0);
        FIELD()
        float Bounciness = 0.f;
        FIELD()
        float Friction = 1.f;
        FIELD()
        float MaxAngularSpeed = 10.f;
        Matrix3 InertiaTensor = Matrix3(0);
        Matrix3 InvInertiaTensor = Matrix3(0);
        uint SleepCounter = 0;
        bool IsSleeping = false;
        Vector3 Acceleration = Vector3(0);
        Vector3 GravityAcceleration = Vector3(0);
        bool IsGrounded = false;
        
        RigidBody() {}

        void ApplyForce(const Vector3& f, const Vector3& deltaPos)
        {
            Force += f;
            Torque += cross(deltaPos, f);
        }

        void Reset()
        {
            Force = Torque = Vector3(0);
        }

        float GetInvMass() { return IsKinematic ? 0.f : 1.0f / Mass; }
    };
}
#include "RigidBody.generated.h"