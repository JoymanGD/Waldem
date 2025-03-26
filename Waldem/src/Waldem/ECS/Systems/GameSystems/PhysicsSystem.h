#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsSystem : ISystem
    {
    public:
        float Gravity = 9.81f;
        
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

                rigidBody.Force += Vector3(0, -Gravity, 0) * rigidBody.Mass;

                rigidBody.Velocity += (rigidBody.Force * rigidBody.InvMass) * deltaTime;
                rigidBody.AngularVelocity += rigidBody.InvInertiaTensor * rigidBody.Torque * deltaTime;
                
                transform.Translate(rigidBody.Velocity * deltaTime);

                if(length(rigidBody.AngularVelocity) > 0.0f)
                {
                    float angle = length(rigidBody.AngularVelocity) * deltaTime;
                    Vector3 axis = normalize(rigidBody.AngularVelocity);
                    Quaternion deltaQuat = angleAxis(angle, axis);;
                    transform.Rotate(deltaQuat);
                }

                rigidBody.Reset();
            }
        }
    };
}