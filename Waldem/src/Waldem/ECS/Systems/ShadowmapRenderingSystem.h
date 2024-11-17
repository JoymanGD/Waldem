#pragma once
#include "System.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API ShadowmapRenderingSystem : ISystem
    {
        PixelShader* DefaultShadowmappingShader = nullptr;
        
    public:
        ShadowmapRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override
        {
            RenderTarget* testShadowMap = nullptr;
            WArray<Matrix4> matrices;
            
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                matrices.Add(transform.GetMatrix());
            }
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                testShadowMap = light.Shadowmap;
                matrices.Add(transform.Inverse());
                matrices.Add(light.Data.Projection);
            }
            
            WArray<Resource> resources;

            if(!matrices.IsEmpty())
                resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, matrices.GetData(), sizeof(Matrix4), matrices.GetSize(), 0));
            
            DefaultShadowmappingShader = Renderer::LoadPixelShader("Shadowmap", resources, testShadowMap);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            WArray<Matrix4> matrices;
            
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                matrices.Add(transform.GetMatrix());
            }
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                matrices.Add(transform.Inverse());
                matrices.Add(light.Data.Projection);
            }
            
            for (auto [entity, modelComponent] : ECSManager->EntitiesWith<ModelComponent>())
            {
                if(!matrices.IsEmpty())
                    DefaultShadowmappingShader->UpdateResourceData("MyConstantBuffer", matrices.GetData());
                
                Renderer::Draw(modelComponent.Model, DefaultShadowmappingShader);
            }
        }
    };
}
