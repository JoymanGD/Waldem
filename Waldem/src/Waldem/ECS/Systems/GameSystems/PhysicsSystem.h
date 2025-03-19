#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsSystem : ISystem
    {
        // Pipeline* CollisionBPPipeline = nullptr;
        // ComputeShader* CollisionBPComputeShader = nullptr;
        // RootSignature* CollisionBPSignature = nullptr;
        //
        // Pipeline* CollisionNPPipeline = nullptr;
        // ComputeShader* CollisionNPComputeShader = nullptr;
        // RootSignature* CollisionNPSignature = nullptr;
        //
        // uint MaxCollisions = 10000;
        // uint MaxObjectsPerThread = 3;

    private:
        
    public:
        PhysicsSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            // for (auto [transformEntity, transform, physicsComponent, meshComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent, MeshComponent>())
            // {
            // }
        }
    };
}