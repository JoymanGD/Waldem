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
            
            WArray<Matrix4> matrices;
            
            WArray<Texture2D*> textures;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                for (auto texture : model.Model->GetTextures())
                    textures.Add(texture);

                matrices.Add(transform.GetMatrix());
            }
            
            for (auto [entity, camera] : ECSManager->EntitiesWith<Camera>())
            {
                matrices.Add(camera.GetViewMatrix());
                matrices.Add(camera.GetProjectionMatrix());
            }
            
            WArray<Resource> resources;

            if(!matrices.IsEmpty())
                resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, matrices.GetData(), sizeof(Matrix4), matrices.GetSize(), 0));
            if(!LightDatas.IsEmpty())
                resources.Add(Resource("LightsBuffer", RTYPE_Buffer, LightDatas.GetData(), sizeof(LightShaderData), LightDatas.GetSize(), 0));
            if(testShadowMap)
                resources.Add(Resource("Shadowmap", testShadowMap, 1));
            if(!textures.IsEmpty())
                resources.Add(Resource("TestTextures", textures, 2));
            resources.Add(Resource("ComparisonSampler", { Sampler( COMPARISON_MIN_MAG_MIP_LINEAR, WRAP, WRAP, WRAP, LESS_EQUAL) }, 1));
            
            DefaultPixelShader = Renderer::LoadPixelShader("Default", resources);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            WArray<Matrix4> matrices;
            
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                matrices.Add(transform.GetMatrix());
            }
            
            for (auto [entity, camera] : ECSManager->EntitiesWith<Camera>())
            {
                matrices.Add(camera.GetViewMatrix());
                matrices.Add(camera.GetProjectionMatrix());
            }
		
            WArray<LightShaderData> LightDatas;
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                LightDatas.Add(lightData);
            }
            
            for (auto [entity, modelComponent] : ECSManager->EntitiesWith<ModelComponent>())
            {
                if(!matrices.IsEmpty())
                    DefaultPixelShader->UpdateResourceData("MyConstantBuffer", matrices.GetData());

                if(!LightDatas.IsEmpty())
                    DefaultPixelShader->UpdateResourceData("LightsBuffer", LightDatas.GetData());
		
                Renderer::Draw(modelComponent.Model, DefaultPixelShader);
            }
        }
    };
}
