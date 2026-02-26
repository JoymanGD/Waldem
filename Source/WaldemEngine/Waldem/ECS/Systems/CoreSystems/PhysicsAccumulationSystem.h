#pragma once
#include "Waldem/Time.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"

namespace Waldem
{
    class WALDEM_API PhysicsAccumulationSystem : public ICoreSystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsAccumulationSystem() {}
        
        void Initialize() override
        {
            ECS::World.observer<Transform, ColliderComponent, RigidBody>("InertiaTensorEvaluatingObserver").event(flecs::OnAdd).each([&](ECS::Entity e, Transform& transform, ColliderComponent& collider, RigidBody& rigidbody)
            {
                rigidbody.InertiaTensor = collider.ComputeInertiaTensor(rigidbody.Mass);
                rigidbody.InvInertiaTensor = inverse(rigidbody.InertiaTensor);
            });
            
            ECS::World.system<Transform, RigidBody>().kind<ECS::OnFixedUpdate>().each([&](ECS::Entity entity, Transform& transform, RigidBody& rigidBody)
            {
                if(rigidBody.IsKinematic) return;

                if(rigidBody.IsSleeping) return;

                rigidBody.Acceleration = Vector3(0);
                rigidBody.GravityAcceleration = Vector3(0);

                auto deltaTime = Time::FixedDeltaTime;

                if(rigidBody.HasGravity)
                {
                    rigidBody.GravityAcceleration = Gravity * deltaTime;
                }

                if(length(rigidBody.Force) > 0.0f)
                {
                    rigidBody.Acceleration = rigidBody.Force * deltaTime;
                }
                
                rigidBody.Velocity += rigidBody.Acceleration;
                rigidBody.Velocity += rigidBody.GravityAcceleration;
                
                auto R = Matrix3(transform.RotationQuat);
                auto transformedInertiaTensor = R * rigidBody.InertiaTensor * transpose(R);
                rigidBody.AngularVelocity += transformedInertiaTensor * rigidBody.Torque * deltaTime;
                
                rigidBody.Velocity *= 1.0f - rigidBody.LinearDamping * deltaTime;
                rigidBody.AngularVelocity *= 1.0f - rigidBody.AngularDamping * deltaTime;

                entity.modified<RigidBody>();
            });
        }
    };
}