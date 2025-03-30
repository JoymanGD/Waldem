#pragma once

namespace Waldem
{
    // struct PhysicsData
    // {
    //     Vector3 Velocity;
    //     float Padding0;
    //     Vector3 Acceleration = { 2, 2, 2 };
    //     float Padding1;
    //     Vector3 Padding2;
    //     float Drag = 3.0f; //size 48
    // };
    //
    // struct PhysicsComponent
    // {
    //     PhysicsData Linear;
    //     PhysicsData Angular;
    //     float Mass;
    //     uint IsKinematic;
    //     uint IsGravity;
    //     uint Padding0;
    // };
    
    struct RigidBody
    {
        bool IsKinematic = false;
        bool Gravity = true;
        Vector3 Velocity = Vector3(0);
        Vector3 AngularVelocity = Vector3(0);
        float Mass = 1.0f;
        float InvMass = 0.0f;
        float LinearDamping = 0.1f;
        float AngularDamping = 0.1f;
        Matrix3 InertiaTensor = Matrix3(0);
        Matrix3 InvInertiaTensor = Matrix3(0);
        Vector3 Force = Vector3(0);
        Vector3 Torque = Vector3(0);
        float Bounciness = 0.f;
        float Friction = 0.f;
        float MaxAngularSpeed = 10.f;

        RigidBody(bool isKinematic, bool gravity, float mass, ColliderComponent* collider) : IsKinematic(isKinematic), Gravity(gravity), Mass(mass)
        {
            UpdateRigidBody(collider);
        }

        void UpdateRigidBody(ColliderComponent* collider)
        {
            if(IsKinematic)
            {
                InertiaTensor = Matrix3(0);
                InvInertiaTensor = Matrix3(0);
                InvMass = 0.0f;
                Mass = 0.0f;
            }
            else
            {
                InertiaTensor = collider->ComputeInertiaTensor(Mass);
                InvInertiaTensor = inverse(InertiaTensor);
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
    };
}