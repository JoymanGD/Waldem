#pragma once
#include "Waldem/Time.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/RigidBody.h"

namespace Waldem
{
    class WALDEM_API PhysicsIntegrationSystem : public ISystem
    {
    public:
        Vector3 Gravity = Vector3(0, -9.81f, 0);
        
        PhysicsIntegrationSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            for (auto [entity, transform, rigidBody] : Manager->EntitiesWith<Transform, RigidBody>())
            {
                if(rigidBody.InvMass <= 0.0f) continue;
                
                if(rigidBody.IsKinematic) continue;
                
                rigidBody.Velocity += Gravity * deltaTime;
                rigidBody.Velocity *= 1.0f - rigidBody.LinearDamping * deltaTime;

                Matrix3 transformedInertiaTensor = Matrix3(transform.Matrix) * rigidBody.InvInertiaTensor * transpose(Matrix3(transform.Matrix));
                rigidBody.AngularVelocity += transformedInertiaTensor * rigidBody.Torque * deltaTime;
                rigidBody.AngularVelocity *= 1.0f - rigidBody.AngularDamping * deltaTime;
            }
        }
    };
}