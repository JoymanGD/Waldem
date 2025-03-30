#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsSystem : ISystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, transform, rigidBody, collider] : ECSManager->EntitiesWith<Transform, RigidBody, ColliderComponent>())
            {
                if(rigidBody.InvMass <= 0.0f) continue;

                if(rigidBody.IsKinematic) continue;

                rigidBody.Velocity += Gravity * deltaTime;
                rigidBody.Velocity *= (1.0f - rigidBody.LinearDamping * deltaTime);
                transform.Translate(rigidBody.Velocity * deltaTime);

                rigidBody.AngularVelocity += rigidBody.InvInertiaTensor * rigidBody.Torque * deltaTime;
                rigidBody.AngularVelocity *= (1.0f - rigidBody.AngularDamping * deltaTime);
                
                if(length(rigidBody.AngularVelocity) > 1e-5f)
                {
                    float angle = length(rigidBody.AngularVelocity) * deltaTime;
                    Vector3 axis = normalize(rigidBody.AngularVelocity);
                    Quaternion deltaQuat = angleAxis(angle, axis);
                    transform.Rotate(normalize(deltaQuat));
                }

                rigidBody.Reset();
            }
        }
    };
}