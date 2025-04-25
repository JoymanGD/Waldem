#pragma once
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API RigidBody : IComponent<RigidBody>
    {
        bool IsKinematic = false;
        bool Gravity = true;
        Vector3 Velocity = Vector3(0);
        Vector3 AngularVelocity = Vector3(0);
        float Mass = 1.0f;
        float InvMass = 0.0f;
        float LinearDamping = 0.9f;
        float AngularDamping = 0.9f;
        Matrix3 InertiaTensor = Matrix3(0);
        Matrix3 InvInertiaTensor = Matrix3(0);
        Vector3 Force = Vector3(0);
        Vector3 Torque = Vector3(0);
        float Bounciness = 0.f;
        float Friction = 0.f;
        float MaxAngularSpeed = 10.f;

        RigidBody() = default;
        
        RigidBody(bool isKinematic, bool gravity, float mass, ColliderComponent* collider) : IsKinematic(isKinematic), Gravity(gravity), Mass(mass)
        {
            UpdateRigidBody(collider);
        }

        void UpdateRigidBody(ColliderComponent* collider)
        {
            if(IsKinematic)
            {
                InvInertiaTensor = Matrix3(0);
                InvMass = Mass = 0.0f;
            }
            else
            {
                InvInertiaTensor = collider->ComputeInertiaTensor(Mass);
                InvMass = Mass > 0.0f ? 1.0f / Mass : 0.0f;
            }
        }

        void ApplyForce(const Vector3& f, const Vector3& deltaPos)
        {
            Force += f;
            Torque += cross(deltaPos, f);
        }

        void Reset()
        {
            Force = Torque = Vector3(0);
        }

        void Serialize(WDataBuffer& outData) override
        {
            outData << IsKinematic;
            outData << Gravity;
            outData << Velocity;
            outData << AngularVelocity;
            outData << Mass;
            outData << InvMass;
            outData << LinearDamping;
            outData << AngularDamping;
            outData << InertiaTensor;
            outData << InvInertiaTensor;
            outData << Force;
            outData << Torque;
            outData << Bounciness;
            outData << Friction;
            outData << MaxAngularSpeed;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> IsKinematic;
            inData >> Gravity;
            inData >> Velocity;
            inData >> AngularVelocity;
            inData >> Mass;
            inData >> InvMass;
            inData >> LinearDamping;
            inData >> AngularDamping;
            inData >> InertiaTensor;
            inData >> InvInertiaTensor;
            inData >> Force;
            inData >> Torque;
            inData >> Bounciness;
            inData >> Friction;
            inData >> MaxAngularSpeed;
        }
    };
}