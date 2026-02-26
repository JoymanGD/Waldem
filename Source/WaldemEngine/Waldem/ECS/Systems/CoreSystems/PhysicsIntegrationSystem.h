#pragma once
#include "Waldem/Time.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsIntegrationSystem : public ICoreSystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsIntegrationSystem() {}
        
        void Initialize() override
        {
            ECS::World.system<Transform, RigidBody>().kind<ECS::OnFixedUpdate>().each([&](ECS::Entity entity, Transform& transform, RigidBody& rigidBody)
            {
                if(rigidBody.IsKinematic) return;

                auto deltaTime = Time::FixedDeltaTime;
                
                transform.Translate(rigidBody.Velocity * deltaTime);
                
                // if(length(rigidBody.AngularVelocity) > 1e-5f)
                // {
                //     float angle = length(rigidBody.AngularVelocity) * deltaTime;
                //     Vector3 axis = normalize(rigidBody.AngularVelocity);
                //     Quaternion deltaQuat = angleAxis(angle, axis);
                //     transform.Rotate(normalize(deltaQuat));
                // }
                                
                entity.modified<Transform>();
                
                rigidBody.Reset();
            });
        }
    };
}