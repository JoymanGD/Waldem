#pragma once
#include "System.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API PhysicsSystem : ISystem
    {
        Pipeline* CollisionPipeline = nullptr;
        ComputeShader* CollisionComputeShader = nullptr;
        RootSignature* CollisionSignature = nullptr;
        
    public:
        PhysicsSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            WArray<PhysicsComponent> physicsComponents;
            WArray<Transform> transforms;
            
            for (auto [transformEntity, transform, physicsComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent>())
            {
                physicsComponents.Add(physicsComponent);
                transforms.Add(transform);
            }
            
            WArray<Resource> CollisionResources;
            CollisionResources.Add(Resource("PhysicsComponents", RTYPE_RWBuffer, physicsComponents.GetData(), sizeof(PhysicsComponent), physicsComponents.GetSize(), 0));
            CollisionResources.Add(Resource("Transforms", RTYPE_Buffer, transforms.GetData(), sizeof(Transform), transforms.GetSize(), 0));
            auto physicsComponentsCount = physicsComponents.Num();
            CollisionResources.Add(Resource("ComponentBuffer", RTYPE_ConstantBuffer, &physicsComponentsCount, sizeof(int), 1, 0));
            CollisionSignature = Renderer::CreateRootSignature(CollisionResources);
            CollisionComputeShader = Renderer::LoadComputeShader("Physics/Collision");
            CollisionPipeline = Renderer::CreateComputePipeline("CollisionPipeline", CollisionSignature, CollisionComputeShader);
        }

        void Update(float deltaTime) override
        {
            WArray<Transform> transforms;
            WArray<PhysicsComponent*> physicsComponents;
            
            for (auto [transformEntity, transform, physicsComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent>())
            {
                transforms.Add(transform);
                physicsComponents.Add(&physicsComponent);
            }
            
            CollisionSignature->UpdateResourceData("Transforms", transforms.GetData());
            
            //Quad drawing pass
            Renderer::SetPipeline(CollisionPipeline);
            Renderer::SetRootSignature(CollisionSignature);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(CollisionComputeShader);
            Point3 groupCount = Point3(glm::ceil(physicsComponents.Num() / (float)numThreads.x), 1, 1);
            Renderer::Compute(groupCount);

            CollisionSignature->ReadbackResourceData("PhysicsComponents", physicsComponents[0]);
        }
    };
}
