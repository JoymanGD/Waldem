#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsUpdateSystem : ISystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsUpdateSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, transform, rigidBody] : ECSManager->EntitiesWith<Transform, RigidBody>())
            {
                if(rigidBody.InvMass <= 0.0f) continue;
                
                if(rigidBody.IsKinematic) continue;
                
                transform.Translate(rigidBody.Velocity * deltaTime);
                
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