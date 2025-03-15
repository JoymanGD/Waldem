#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "..\..\Components\EditorCamera.h"
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
        Texture2D* DummyTexture = nullptr;
        //GBuffer pass
        Pipeline* GBufferPipeline = nullptr;
        RootSignature* GBufferRootSignature = nullptr;
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        //Deferred rendering pass
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        RootSignature* DeferredRenderingRootSignature = nullptr;
        Point3 GroupCount;
        
    public:
        DeferredRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager)
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            int width = 1;
            int height = 1;

            TextureFormat format = TextureFormat::R32G32B32_FLOAT;

            DummyTexture = Renderer::CreateTexture("DummyTexture", width, height, format, image_data); 
        }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            
            //Common resources
            auto constantBufferResource = Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 4, 0);
            
            //GBuffer pass
            WArray<MaterialShaderAttribute> materialAttributes;
            
            WArray<Texture2D*> materialTextures { DummyTexture };
            
            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                if(!mesh.Mesh->CurrentMaterial->HasDiffuseTexture())
                {
                    materialAttributes.Add(MaterialShaderAttribute()); //by default all texture indices = -1, so we can just add empty MaterialAttribute
                    continue;
                }

                int diffuseId = -1, normalId = -1, ormId = -1, clearCoat = -1;
                
                diffuseId = materialTextures.Add(mesh.Mesh->CurrentMaterial->GetDiffuseTexture());
                
                if(mesh.Mesh->CurrentMaterial->HasDiffuseTexture())
                    normalId = materialTextures.Add(mesh.Mesh->CurrentMaterial->GetNormalTexture());
                if(mesh.Mesh->CurrentMaterial->HasDiffuseTexture())
                    ormId = materialTextures.Add(mesh.Mesh->CurrentMaterial->GetORMTexture());

                materialAttributes.Add(MaterialShaderAttribute{ diffuseId, normalId, ormId, clearCoat, mesh.Mesh->CurrentMaterial->Albedo, mesh.Mesh->CurrentMaterial->Metallic, mesh.Mesh->CurrentMaterial->Roughness });
            }

            WArray<Matrix4> worldTransforms;
            
            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
            }
            
            WArray<Resource> gBufferPassResources;
            gBufferPassResources.Add(constantBufferResource);
            gBufferPassResources.Add(Resource("RootConstants", RTYPE_Constant, nullptr, sizeof(uint32_t), sizeof(uint32_t), 1));
            
            gBufferPassResources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            gBufferPassResources.Add(Resource("MaterialAttributes", RTYPE_Buffer, materialAttributes.GetData(), sizeof(MaterialShaderAttribute), materialAttributes.GetSize(), 1));
            gBufferPassResources.Add(Resource("MaterialTextures", materialTextures, 2));

            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            ColorRT = resourceManager->GetRenderTarget("ColorRT");
            ORMRT = resourceManager->GetRenderTarget("ORMRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT = resourceManager->GetRenderTarget("DepthRT");
            GBufferRootSignature = Renderer::CreateRootSignature(gBufferPassResources);
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            GBufferPipeline = Renderer::CreateGraphicPipeline("GBufferPipeline",
                                                            GBufferRootSignature,
                                                            GBufferPixelShader,
                                                            { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), ORMRT->GetFormat(), MeshIDRT->GetFormat() },
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

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
            deferredRenderingPassResources.Add(Resource("ORMRT", ORMRT, 5));
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
            Matrix4 matrices[4];
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                matrices[0] = camera.ViewMatrix;
                matrices[1] = camera.ProjectionMatrix;
                matrices[2] = cameraTransform.Matrix;
                matrices[3] = inverse(camera.ProjectionMatrix);
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
            Renderer::ResourceBarrier(ORMRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);
            Renderer::SetRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT }, DepthRT);
            Renderer::ClearRenderTarget(WorldPositionRT);
            Renderer::ClearRenderTarget(NormalRT);
            Renderer::ClearRenderTarget(ColorRT);
            Renderer::ClearRenderTarget(ORMRT);
            Renderer::ClearRenderTarget(MeshIDRT);
            Renderer::ClearDepthStencil(DepthRT);
            
            uint32_t meshID = 0;

            for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
            {
                auto transformedBBox = mesh.Mesh->BBox.GetTransformed(transform.Matrix);

                //Frustrum culling
                if(transformedBBox.IsInFrustum(frustrumPlanes))
                {
                    GBufferRootSignature->UpdateResourceData("RootConstants", &meshID); 
                    Renderer::Draw(mesh.Mesh);
                }

                meshID++;
            }
            
            Renderer::SetRenderTargets({});

            //Deferred rendering pass
            Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
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
            int hoveredEntityId = 0;
            DeferredRenderingRootSignature->ReadbackResourceData("HoveredMeshes", &hoveredEntityId);
            Editor::HoveredIntityID = hoveredEntityId - 1;
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}
