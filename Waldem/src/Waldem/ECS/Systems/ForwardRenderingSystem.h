#pragma once
#include "System.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API ForwardRenderingSystem : ISystem
    {
        PixelShader* DefaultPixelShader = nullptr;
    public:
        ForwardRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override
        {
            RenderTarget* testShadowMap = nullptr;
		
            WArray<LightShaderData> LightDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                testShadowMap = light.Shadowmap;
                LightDatas.Add(lightData);
            }
            
            WArray<Texture2D*> textures;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                for (auto texture : model.Model->GetTextures())
                    textures.Add(texture);

                break;
            }

            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.GetMatrix());
            }
            
            WArray<Resource> resources;

            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            resources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            
            if(!LightDatas.IsEmpty())
                resources.Add(Resource("LightsBuffer", RTYPE_Buffer, nullptr, sizeof(LightShaderData), LightDatas.GetSize(), 0));
            if(testShadowMap)
                resources.Add(Resource("Shadowmap", testShadowMap, 1));
            if(!worldTransforms.IsEmpty())
                resources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 2));
            if(!textures.IsEmpty())
                resources.Add(Resource("TestTextures", textures, 3));
            resources.Add(Resource("ComparisonSampler", { Sampler( COMPARISON_MIN_MAG_MIP_LINEAR, WRAP, WRAP, WRAP, LESS_EQUAL) }, 1));
            
            DefaultPixelShader = Renderer::LoadPixelShader("Default", resources);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            WArray<LightShaderData> LightDatas;
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                LightDatas.Add(lightData);
            }

            if(!LightDatas.IsEmpty())
                DefaultPixelShader->UpdateResourceData("LightsBuffer", LightDatas.GetData());
		
            Matrix4 matrices[2];
            for (auto [entity, camera, mainCamera] : ECSManager->EntitiesWith<Camera, MainCamera>())
            {
                matrices[0] = camera.GetViewMatrix();
                matrices[1] = camera.GetProjectionMatrix();
                DefaultPixelShader->UpdateResourceData("MyConstantBuffer", matrices);

                break;
            }

            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.GetMatrix());
            }

            DefaultPixelShader->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
            
            Renderer::BeginDraw(DefaultPixelShader);

            uint32_t modelID = 0;
            
            for (auto [entity, modelComponent, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                DefaultPixelShader->UpdateResourceData("RootConstants", &modelID);
                Renderer::Draw(modelComponent.Model);
                modelID++;
            }
            
            Renderer::EndDraw(DefaultPixelShader);
        }
    };
}
