#pragma once
#include "System.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API DeferredRenderingSystem : ISystem
    {
        //GBuffer pass
        Pipeline* GBufferPipeline = nullptr;
        RootSignature* GBufferRootSignature = nullptr;
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* AlbedoRT = nullptr;
        //Deferred lighting pass
        Pipeline* DeferredLightingPipeline = nullptr;
        PixelShader* DeferredLightingPixelShader = nullptr;
        RootSignature* DeferredLightingRootSignature = nullptr;
        Quad FullscreenQuad = {};
        
    public:
        DeferredRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override
        {
            //GBuffer pass
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
            
            WArray<Resource> gBufferPassResources;
            gBufferPassResources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            gBufferPassResources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            if(!worldTransforms.IsEmpty())
                gBufferPassResources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            if(!textures.IsEmpty())
                gBufferPassResources.Add(Resource("TestTextures", textures, 1));

            WorldPositionRT = Renderer::CreateRenderTarget("WorldPositionRT", sceneData->Window->GetWidth(), sceneData->Window->GetHeight(), TextureFormat::R32G32B32A32_FLOAT);
            NormalRT = Renderer::CreateRenderTarget("NormalRT", sceneData->Window->GetWidth(), sceneData->Window->GetHeight(), TextureFormat::R16G16B16A16_FLOAT);
            AlbedoRT = Renderer::CreateRenderTarget("AlbedoRT", sceneData->Window->GetWidth(), sceneData->Window->GetHeight(), TextureFormat::R8G8B8A8_UNORM);
            GBufferRootSignature = Renderer::CreateRootSignature(gBufferPassResources);
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            GBufferPipeline = Renderer::CreatePipeline("GBufferPipeline", { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), AlbedoRT->GetFormat() }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, GBufferRootSignature, GBufferPixelShader);

            //Deferred lighting pass
            RenderTarget* testShadowMap = nullptr;
            WArray<LightShaderData> LightDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                testShadowMap = light.Shadowmap;
                LightDatas.Add(lightData);
            }
            WArray<Resource> deferredLightingPassResources;
            deferredLightingPassResources.Add(Resource("ComparisonSampler", { Sampler( COMPARISON_MIN_MAG_MIP_LINEAR, WRAP, WRAP, WRAP, LESS_EQUAL) }, 1));
            if(!LightDatas.IsEmpty())
                deferredLightingPassResources.Add(Resource("LightsBuffer", RTYPE_Buffer, nullptr, sizeof(LightShaderData), LightDatas.GetSize(), 0));
            if(testShadowMap)
                deferredLightingPassResources.Add(Resource("Shadowmap", testShadowMap, 1));
            deferredLightingPassResources.Add(Resource("WorldPosition", WorldPositionRT, 2));
            deferredLightingPassResources.Add(Resource("Normal", NormalRT, 3));
            deferredLightingPassResources.Add(Resource("Albedo", AlbedoRT, 4));
            
            DeferredLightingRootSignature = Renderer::CreateRootSignature(deferredLightingPassResources);
            DeferredLightingPixelShader = Renderer::LoadPixelShader("DeferredRendering");
            DeferredLightingPipeline = Renderer::CreatePipeline("DeferredLightingPipeline", { TextureFormat::R8G8B8A8_UNORM }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, DeferredLightingRootSignature, DeferredLightingPixelShader);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            //GBuffer pass
            WArray<FrustumPlane> frustrumPlanes;
            Matrix4 matrices[2];
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, MainCamera, Transform>())
            {
                matrices[0] = camera.GetViewMatrix();
                matrices[1] = camera.GetProjectionMatrix();
                GBufferRootSignature->UpdateResourceData("MyConstantBuffer", matrices);
                frustrumPlanes = camera.ExtractFrustumPlanes();

                break;
            }

            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.GetMatrix());
            }

            GBufferRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
            Renderer::SetPipeline(GBufferPipeline);
            Renderer::SetRootSignature(GBufferRootSignature);

            Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(AlbedoRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::SetRenderTargets({ WorldPositionRT, NormalRT, AlbedoRT });
            Renderer::ClearRenderTarget(WorldPositionRT);
            Renderer::ClearRenderTarget(NormalRT);
            Renderer::ClearRenderTarget(AlbedoRT);
            
            uint32_t modelID = 0;
            for (auto [entity, modelComponent, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                GBufferRootSignature->UpdateResourceData("RootConstants", &modelID);
                for (auto mesh : modelComponent.Model->GetMeshes())
                {
                    auto transformedBBox = mesh->BBox.Transform(transform.GetMatrix());

                    //Frustrum culling
                    if(transformedBBox.IsInFrustum(frustrumPlanes))
                    {
                        Renderer::Draw(mesh);
                    }
                }
                modelID++;
            }
            Renderer::SetRenderTargets({});
            
            //Deferred lighting pass
            Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(AlbedoRT, RENDER_TARGET, ALL_SHADER_RESOURCE);

            WArray<LightShaderData> LightDatas;
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                LightDatas.Add(lightData);
            }

            if(!LightDatas.IsEmpty())
                DeferredLightingRootSignature->UpdateResourceData("LightsBuffer", LightDatas.GetData());
            Renderer::SetPipeline(DeferredLightingPipeline);
            Renderer::SetRootSignature(DeferredLightingRootSignature);
            Renderer::Draw(&FullscreenQuad);
            
            //Post process pass
        }
    };
}
