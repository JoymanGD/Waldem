#pragma once
#include "System.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API DeferredRenderingSystem : ISystem
    {
        RenderTarget* TargetRT = nullptr;
        //GBuffer pass
        Pipeline* GBufferPipeline = nullptr;
        RootSignature* GBufferRootSignature = nullptr;
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* MetalRoughnessRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        //Deferred rendering pass
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        RootSignature* DeferredRenderingRootSignature = nullptr;
        Point3 GroupCount;
        
    public:
        DeferredRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            
            //Common resources
            auto constantBufferResource = Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0);
            
            //GBuffer pass
            WArray<Texture2D*> diffuseTextures;
            WArray<Texture2D*> normalTextures;
            WArray<Texture2D*> metalRoughnessTextures;
            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                if(mesh.Mesh->CurrentMaterial->GetDiffuseTexture() == nullptr)
                    continue;
                    diffuseTextures.Add(mesh.Mesh->CurrentMaterial->GetDiffuseTexture());
                if(mesh.Mesh->CurrentMaterial->GetNormalTexture() != nullptr)
                    normalTextures.Add(mesh.Mesh->CurrentMaterial->GetNormalTexture());
                if(mesh.Mesh->CurrentMaterial->GetMetalRoughnessTexture() != nullptr)
                    metalRoughnessTextures.Add(mesh.Mesh->CurrentMaterial->GetMetalRoughnessTexture());
            }

            WArray<Matrix4> worldTransforms;
            
            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
            }
            
            WArray<Resource> gBufferPassResources;
            gBufferPassResources.Add(constantBufferResource);
            gBufferPassResources.Add(Resource("RootConstants", RTYPE_Constant, nullptr, sizeof(uint32_t), sizeof(uint32_t), 1));
            if(!worldTransforms.IsEmpty())
                gBufferPassResources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            if(!diffuseTextures.IsEmpty())
                gBufferPassResources.Add(Resource("DiffuseTextures", diffuseTextures, 1));
            if(!normalTextures.IsEmpty())
                gBufferPassResources.Add(Resource("NormalTextures", normalTextures, 1025));
            if(!metalRoughnessTextures.IsEmpty())
                gBufferPassResources.Add(Resource("MetalRoughnessTextures", metalRoughnessTextures, 2049));

            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            ColorRT = resourceManager->GetRenderTarget("ColorRT");
            MetalRoughnessRT = resourceManager->GetRenderTarget("MetalRoughnessRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT = resourceManager->GetRenderTarget("DepthRT");
            GBufferRootSignature = Renderer::CreateRootSignature(gBufferPassResources);
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            GBufferPipeline = Renderer::CreateGraphicPipeline("GBufferPipeline", { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), MetalRoughnessRT->GetFormat(), MeshIDRT->GetFormat() }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, GBufferRootSignature, GBufferPixelShader);

            //Deferred rendering pass
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
            deferredRenderingPassResources.Add(Resource("Color", ColorRT, 4));
            deferredRenderingPassResources.Add(Resource("MetalRoughness", MetalRoughnessRT, 5));
            deferredRenderingPassResources.Add(Resource("MeshIDRT", MeshIDRT, 6));
            deferredRenderingPassResources.Add(Resource("DepthRT", DepthRT, 7));
            deferredRenderingPassResources.Add(Resource("TargetRT", TargetRT, 0, true));
            deferredRenderingPassResources.Add(Resource("HoveredMeshes", RTYPE_RWBuffer, nullptr, sizeof(int), sizeof(int), 1));
            deferredRenderingPassResources.Add(constantBufferResource);
            deferredRenderingPassResources.Add(Resource("RootConstants", RTYPE_Constant, nullptr, sizeof(float) * 2, sizeof(float) * 2, 1));
            DeferredRenderingRootSignature = Renderer::CreateRootSignature(deferredRenderingPassResources);
            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingRootSignature, DeferredRenderingComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
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
                DeferredRenderingRootSignature->UpdateResourceData("MyConstantBuffer", matrices);
                frustrumPlanes = camera.ExtractFrustumPlanes();

                break;
            }
            WArray<Matrix4> worldTransforms;
            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
            }
            GBufferRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
            Renderer::SetPipeline(GBufferPipeline);
            Renderer::SetRootSignature(GBufferRootSignature);
            Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(ColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(MetalRoughnessRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);
            Renderer::SetRenderTargets({ WorldPositionRT, NormalRT, ColorRT, MetalRoughnessRT, MeshIDRT }, DepthRT);
            Renderer::ClearRenderTarget(WorldPositionRT);
            Renderer::ClearRenderTarget(NormalRT);
            Renderer::ClearRenderTarget(ColorRT);
            Renderer::ClearRenderTarget(MetalRoughnessRT);
            Renderer::ClearRenderTarget(MeshIDRT);
            Renderer::ClearDepthStencil(DepthRT);
            uint32_t meshId = 0;
            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                GBufferRootSignature->UpdateResourceData("RootConstants", &meshId);
                
                auto transformedBBox = mesh.Mesh->BBox.Transform(transform.Matrix);

                //Frustrum culling
                if(transformedBBox.IsInFrustum(frustrumPlanes))
                {
                    Renderer::Draw(mesh.Mesh);
                }
                meshId++;
            }
            Renderer::SetRenderTargets({});

            //Deferred rendering pass
            Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(MetalRoughnessRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::SetPipeline(DeferredRenderingPipeline);
            Renderer::SetRootSignature(DeferredRenderingRootSignature);
            WArray<LightShaderData> LightDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                LightDatas.Add(lightData);
            }
            if(!LightDatas.IsEmpty())
                DeferredRenderingRootSignature->UpdateResourceData("LightsBuffer", LightDatas.GetData());
            auto mousePos = Input::GetMousePos();
            DeferredRenderingRootSignature->UpdateResourceData("RootConstants", &mousePos);
            Renderer::Compute(GroupCount);
            DeferredRenderingRootSignature->ReadbackResourceData("HoveredMeshes", &Editor::HoveredIntityID);
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}
