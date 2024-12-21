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
    struct BloomParams
    {
        float BrightThreshold = 0.5f;
        float BloomIntensity = .6f;
        Vector2 TexelSize = Vector2(1.f / 1280.f, 1.f / 720.f);
    };

    class WALDEM_API DeferredRenderingSystem : ISystem
    {
        //GBuffer pass
        Pipeline* GBufferPipeline = nullptr;
        RootSignature* GBufferRootSignature = nullptr;
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* AlbedoRT = nullptr;
        //Deferred rendering pass
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        RootSignature* DeferredRenderingRootSignature = nullptr;
        RenderTarget* DeferredRenderingRenderTarget = nullptr;
        Point3 GroupCount;
        //Post process pass
        Pipeline* PostProcessPipeline = nullptr;
        ComputeShader* PostProcessComputeShader = nullptr;
        RootSignature* PostProcessRootSignature = nullptr;
        RenderTarget* PostProcessRenderTarget = nullptr;
        //Quad draw pass
        Pipeline* QuadDrawPipeline = nullptr;
        PixelShader* QuadDrawPixelShader = nullptr;
        RootSignature* QuadDrawRootSignature = nullptr;
        Quad FullscreenQuad = {};
        
    public:
        DeferredRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            //Common resources
            auto constantBufferResource = Resource("MyConstantBuffer", RTYPE_ConstantBuffer, &resolution, sizeof(Vector2), sizeof(Vector2), 0);
            
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
                worldTransforms.Add(transform.Matrix);
            }
            
            WArray<Resource> gBufferPassResources;
            gBufferPassResources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            gBufferPassResources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            if(!worldTransforms.IsEmpty())
                gBufferPassResources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            if(!textures.IsEmpty())
                gBufferPassResources.Add(Resource("TestTextures", textures, 1));

            WorldPositionRT = Renderer::CreateRenderTarget("WorldPositionRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);
            NormalRT = Renderer::CreateRenderTarget("NormalRT", resolution.x, resolution.y, TextureFormat::R16G16B16A16_FLOAT);
            AlbedoRT = Renderer::CreateRenderTarget("AlbedoRT", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
            GBufferRootSignature = Renderer::CreateRootSignature(gBufferPassResources);
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            GBufferPipeline = Renderer::CreateGraphicPipeline("GBufferPipeline", { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), AlbedoRT->GetFormat() }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, GBufferRootSignature, GBufferPixelShader);

            //Deferred rendering pass
            DeferredRenderingRenderTarget = Renderer::CreateRenderTarget("DeferredRenderingRenderTarget", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
            Renderer::ResourceBarrier(DeferredRenderingRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            WArray<Resource> deferredRenderingPassResources;
            RenderTarget* testShadowMap = nullptr;
            WArray<LightShaderData> LightDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                testShadowMap = light.Shadowmap;
                LightDatas.Add(lightData);
            }
            deferredRenderingPassResources.Add(Resource("ComparisonSampler", { Sampler( COMPARISON_MIN_MAG_MIP_LINEAR, WRAP, WRAP, WRAP, LESS_EQUAL) }, 1));
            if(!LightDatas.IsEmpty())
                deferredRenderingPassResources.Add(Resource("LightsBuffer", RTYPE_Buffer, nullptr, sizeof(LightShaderData), LightDatas.GetSize(), 0));
            if(testShadowMap)
                deferredRenderingPassResources.Add(Resource("Shadowmap", testShadowMap, 1));
            deferredRenderingPassResources.Add(Resource("WorldPosition", WorldPositionRT, 2));
            deferredRenderingPassResources.Add(Resource("Normal", NormalRT, 3));
            deferredRenderingPassResources.Add(Resource("Albedo", AlbedoRT, 4));
            deferredRenderingPassResources.Add(Resource("DeferredRenderingRenderTarget", DeferredRenderingRenderTarget, 0, true));
            deferredRenderingPassResources.Add(constantBufferResource);
            DeferredRenderingRootSignature = Renderer::CreateRootSignature(deferredRenderingPassResources);
            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingRootSignature, DeferredRenderingComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);

            //Post process pass
            PostProcessRenderTarget = Renderer::CreateRenderTarget("PostProcessRenderTarget", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
            Renderer::ResourceBarrier(PostProcessRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            WArray<Resource> postProcessPassResources;
            postProcessPassResources.Add(Resource("DeferredRenderingRenderTarget", DeferredRenderingRenderTarget, 0));
            postProcessPassResources.Add(Resource("PostProcessRenderTarget", PostProcessRenderTarget, 0, true));
            postProcessPassResources.Add(constantBufferResource);
            BloomParams bloomParams = {};
            postProcessPassResources.Add(Resource("BloomParams", RTYPE_ConstantBuffer, &bloomParams, sizeof(BloomParams), sizeof(BloomParams), 1));
            
            PostProcessRootSignature = Renderer::CreateRootSignature(postProcessPassResources);
            PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
            PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessRootSignature, PostProcessComputeShader);
            
            //Quad draw pass
            WArray<Resource> QuadDrawPassResources;
            QuadDrawPassResources.Add(Resource("PostProcessRenderTarget", PostProcessRenderTarget, 0));
            QuadDrawRootSignature = Renderer::CreateRootSignature(QuadDrawPassResources);
            QuadDrawPixelShader = Renderer::LoadPixelShader("QuadDraw");
            QuadDrawPipeline = Renderer::CreateGraphicPipeline("QuadDrawPipeline", { TextureFormat::R8G8B8A8_UNORM }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, QuadDrawRootSignature, QuadDrawPixelShader);
        }

        void Update(float deltaTime) override
        {
            //GBuffer pass
            WArray<FrustumPlane> frustrumPlanes;
            Matrix4 matrices[2];
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, MainCamera, Transform>())
            {
                matrices[0] = camera.ViewMatrix;
                matrices[1] = camera.ProjectionMatrix;
                GBufferRootSignature->UpdateResourceData("MyConstantBuffer", matrices);
                frustrumPlanes = camera.ExtractFrustumPlanes();

                break;
            }

            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
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
                    auto transformedBBox = mesh->BBox.Transform(transform.Matrix);

                    //Frustrum culling
                    if(transformedBBox.IsInFrustum(frustrumPlanes))
                    {
                        Renderer::Draw(mesh);
                    }
                }
                modelID++;
            }
            Renderer::SetRenderTargets({});

            //Deferred rendering pass
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
                DeferredRenderingRootSignature->UpdateResourceData("LightsBuffer", LightDatas.GetData());
            Renderer::SetPipeline(DeferredRenderingPipeline);
            Renderer::SetRootSignature(DeferredRenderingRootSignature);
            Renderer::Compute(GroupCount);

            //Post process pass
            Renderer::ResourceBarrier(DeferredRenderingRenderTarget, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            Renderer::SetPipeline(PostProcessPipeline);
            Renderer::SetRootSignature(PostProcessRootSignature);
            Renderer::Compute(GroupCount);
            Renderer::ResourceBarrier(DeferredRenderingRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            
            //Quad drawing pass
            Renderer::ResourceBarrier(PostProcessRenderTarget, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            Renderer::SetPipeline(QuadDrawPipeline);
            Renderer::SetRootSignature(QuadDrawRootSignature);
            Renderer::Draw(&FullscreenQuad);
            Renderer::ResourceBarrier(PostProcessRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
        }
    };
}
