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
            
            // WArray<Matrix4> matrices;
            // for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            // {
            //     matrices.Add(transform.GetMatrix());
            // }
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                testShadowMap = light.Shadowmap;
                // matrices.Add(transform.Inverse());
                // matrices.Add(light.Data.Projection);
            }
            
            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.GetMatrix());
            }
            
            WArray<Resource> resources;

            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            resources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            if(!worldTransforms.IsEmpty())
                resources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            
            DefaultShadowmappingShader = Renderer::LoadPixelShader("Shadowmap", resources, testShadowMap);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                Matrix4 matrices[2];
                matrices[0] = transform.Inverse();
                matrices[1] = light.Data.Projection;
            
                DefaultShadowmappingShader->UpdateResourceData("MyConstantBuffer", matrices);

                WArray<Matrix4> worldTransforms;
                for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
                {
                    worldTransforms.Add(transform.GetMatrix());
                }

                DefaultShadowmappingShader->UpdateResourceData("WorldTransforms", worldTransforms.GetData());

                uint32_t modelID = 0;

                Renderer::BeginDraw(DefaultShadowmappingShader);
                
                for (auto [entity, modelComponent, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
                {
                    DefaultShadowmappingShader->UpdateResourceData("RootConstants", &modelID);
                    
                    Renderer::Draw(modelComponent.Model);

                    modelID++;
                }

                Renderer::EndDraw(DefaultShadowmappingShader);
            }
        }
    };
}
