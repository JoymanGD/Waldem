#pragma once
#include "Waldem/Time.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsUpdateSystem : public ISystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsUpdateSystem() {}
        
        void Initialize(InputManager* inputManager) override
        {
            ECS::World.system<Transform, RigidBody>().kind(flecs::OnUpdate).each([&](Transform& transform, RigidBody& rigidBody)
            {
                if(rigidBody.InvMass <= 0.0f) return;
                
                if(rigidBody.IsKinematic) return;

                auto deltaTime = Time::DeltaTime;
                
                transform.Translate(rigidBody.Velocity * deltaTime);
                
                if(length(rigidBody.AngularVelocity) > 1e-5f)
                {
                    float angle = length(rigidBody.AngularVelocity) * deltaTime;
                    Vector3 axis = normalize(rigidBody.AngularVelocity);
                    Quaternion deltaQuat = angleAxis(angle, axis);
                    transform.Rotate(normalize(deltaQuat));
                }
                
                rigidBody.Reset();
            });
        }
    };
}