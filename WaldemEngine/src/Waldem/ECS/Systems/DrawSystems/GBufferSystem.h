#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"

namespace Waldem
{
    class WALDEM_API GBufferSystem : public DrawSystem
    {
        RenderTarget* TargetRT = nullptr;
        Texture2D* DummyTexture = nullptr;
        //GBuffer pass
        Pipeline* GBufferPipeline = nullptr;
        RootSignature* GBufferRootSignature = nullptr;
        CommandSignature* GBufferCommandSignature = nullptr;
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        Buffer* IndirectBuffer = nullptr;
        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
        WArray<IndirectCommand> IndirectDrawIndexedArgsArray;
        
    public:
        GBufferSystem(ECSManager* eCSManager) : DrawSystem(eCSManager)
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            int width = 1;
            int height = 1;

            TextureFormat format = TextureFormat::R8G8B8A8_UNORM;

            DummyTexture = Renderer::CreateTexture("DummyTexture", width, height, format, sizeof(Vector4), image_data); 
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            if(Manager->EntitiesWith<MeshComponent, Transform>().Count() <= 0)
                return;
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            
            //Common resources
            auto constantBufferResource = GraphicResource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 4, 1);
            
            //GBuffer pass
            WArray<MaterialShaderAttribute> materialAttributes;
            
            WArray<Matrix4> worldTransforms;

            WArray materialTextures { DummyTexture };

            uint startIndexLocation = 0;
            int baseVertexLocation = 0;

            uint32_t meshID = 0;
            
            WArray<Vertex> vertices;
            WArray<uint> indices;
            
            for (auto [entity, meshComponent, transform] : Manager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
                vertices.AddRange(meshComponent.Mesh->VertexData);
                indices.AddRange(meshComponent.Mesh->IndexData);
                
                uint indexCountPerInstance = (uint)meshComponent.Mesh->IndexData.Num();

                IndirectDrawIndexedArgsArray.Add(
                {
                    meshID,
                    {
                        indexCountPerInstance,
                        1,
                        startIndexLocation,
                        baseVertexLocation,
                        0
                    }
                });

                startIndexLocation += indexCountPerInstance;
                baseVertexLocation += (int)meshComponent.Mesh->VertexData.Num();
                meshID++;
                
                if(!meshComponent.Mesh->CurrentMaterial->HasDiffuseTexture())
                {
                    materialAttributes.Add(MaterialShaderAttribute()); //by default all texture indices = -1, so we can just add empty MaterialAttribute
                    continue;
                }

                int diffuseId = -1, normalId = -1, ormId = -1, clearCoat = -1;
                
                diffuseId = materialTextures.Add(meshComponent.Mesh->CurrentMaterial->GetDiffuseTexture());
                
                if(meshComponent.Mesh->CurrentMaterial->HasNormalTexture())
                    normalId = materialTextures.Add(meshComponent.Mesh->CurrentMaterial->GetNormalTexture());
                if(meshComponent.Mesh->CurrentMaterial->HasORMTexture())
                    ormId = materialTextures.Add(meshComponent.Mesh->CurrentMaterial->GetORMTexture());

                materialAttributes.Add(MaterialShaderAttribute{ diffuseId, normalId, ormId, clearCoat, meshComponent.Mesh->CurrentMaterial->Albedo, meshComponent.Mesh->CurrentMaterial->Metallic, meshComponent.Mesh->CurrentMaterial->Roughness });
            }

            IndirectBuffer = ResourceManager::CreateBuffer("IndirectDrawBuffer", BufferType::IndirectBuffer, IndirectDrawIndexedArgsArray.GetData(), IndirectDrawIndexedArgsArray.GetSize(), sizeof(IndirectCommand));
            VertexBuffer = ResourceManager::CreateBuffer("VertexBuffer", BufferType::VertexBuffer, vertices.GetData(), vertices.GetSize(), sizeof(Vertex));
            IndexBuffer = ResourceManager::CreateBuffer("IndexBuffer", BufferType::IndexBuffer, indices.GetData(), indices.GetSize(), sizeof(uint));
            
            WArray<GraphicResource> gBufferPassResources;
            gBufferPassResources.Add(GraphicResource("RootConstants", RTYPE_Constant, nullptr, sizeof(uint32_t), sizeof(uint32_t), 0));
            gBufferPassResources.Add(constantBufferResource);
            gBufferPassResources.Add(GraphicResource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            gBufferPassResources.Add(GraphicResource("MaterialAttributes", RTYPE_Buffer, materialAttributes.GetData(), sizeof(MaterialShaderAttribute), materialAttributes.GetSize(), 1));
            gBufferPassResources.Add(GraphicResource("MaterialTextures", materialTextures, 2));

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

            GBufferCommandSignature = Renderer::CreateCommandSignature(GBufferRootSignature);

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(GBufferCommandSignature) GBufferCommandSignature->Destroy();
            if(GBufferRootSignature) GBufferRootSignature->Destroy();
            if(GBufferPixelShader) GBufferPixelShader->Destroy();
            if(GBufferPipeline) GBufferPipeline->Destroy();
            if(IndirectBuffer) IndirectBuffer->Destroy();
            if(VertexBuffer) VertexBuffer->Destroy();
            if(IndexBuffer) IndexBuffer->Destroy();

            IndirectDrawIndexedArgsArray.Clear();
            
            IsInitialized = false;
        }

        void Update(float deltaTime) override
        {
            if(!IsInitialized)
                return;
            
            Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(ColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(ORMRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
            Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);
            
            //GBuffer pass
            WArray<FrustumPlane> frustrumPlanes;
            Matrix4 matrices[4];
            for (auto [entity, camera, mainCamera, cameraTransform] : Manager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                matrices[0] = camera.ViewMatrix;
                matrices[1] = camera.ProjectionMatrix;
                matrices[2] = cameraTransform.Matrix;
                matrices[3] = inverse(camera.ProjectionMatrix);
                GBufferRootSignature->UpdateResourceData("MyConstantBuffer", matrices);
                frustrumPlanes = camera.ExtractFrustumPlanes();

                break;
            }
            WArray<Matrix4> worldTransforms;
            for (auto [entity, mesh, transform] : Manager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
            }
            GBufferRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
            Renderer::SetPipeline(GBufferPipeline);
            Renderer::SetRootSignature(GBufferRootSignature);
            Renderer::SetRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT }, DepthRT);
            Renderer::ClearRenderTarget(WorldPositionRT);
            Renderer::ClearRenderTarget(NormalRT);
            Renderer::ClearRenderTarget(ColorRT);
            Renderer::ClearRenderTarget(ORMRT);
            Renderer::ClearRenderTarget(MeshIDRT);
            Renderer::ClearDepthStencil(DepthRT);
            
            Renderer::SetVertexBuffers(VertexBuffer, 1);
            Renderer::SetIndexBuffer(IndexBuffer);
            Renderer::DrawIndirect(GBufferCommandSignature, IndirectDrawIndexedArgsArray.Num(), IndirectBuffer);
            
            Renderer::SetRenderTargets({});

            Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
        }

        void OnResize(Vector2 size) override
        {
        }
    };
}
