#pragma once
#include "Waldem/Time.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"

namespace Waldem
{
    class WALDEM_API PhysicsIntegrationSystem : public ICoreSystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsIntegrationSystem() {}
        
        void Initialize() override
        {
            ECS::World.system<Transform, RigidBody>().kind(flecs::OnUpdate).each([&](Transform& transform, RigidBody& rigidBody)
            {
                if(rigidBody.InvMass <= 0.0f) return;
                
                if(rigidBody.IsKinematic) return;

                auto deltaTime = Time::DeltaTime;
                
                rigidBody.Velocity += Gravity * deltaTime;
                rigidBody.Velocity *= 1.0f - rigidBody.LinearDamping * deltaTime;

                Matrix3 transformedInertiaTensor = Matrix3(transform.Matrix) * rigidBody.InvInertiaTensor * transpose(Matrix3(transform.Matrix));
                rigidBody.AngularVelocity += transformedInertiaTensor * rigidBody.Torque * deltaTime;
                rigidBody.AngularVelocity *= 1.0f - rigidBody.AngularDamping * deltaTime;
            });
        }
    };
}