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
    struct GBufferBuffers
    {
        uint WorldTransforms;
        uint MaterialAttributes;
    };
    
    struct GBufferSceneData
    {
        Matrix4 ViewMatrix;
        Matrix4 ProjectionMatrix;
        Matrix4 WorldMatrix;
        Matrix4 InverseProjectionMatrix;
    };

    struct GBufferRootConstants
    {
        uint BuffersIndicesBuffer;
        uint SceneDataBuffer;
    };
    
    class WALDEM_API GBufferSystem : public DrawSystem
    {
        RenderTarget* TargetRT = nullptr;
        Texture2D* DummyTexture = nullptr;
        //GBuffer pass
        Pipeline* GBufferPipeline = nullptr;
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
        Buffer* WorldTransformsBuffer = nullptr;
        Buffer* MaterialAttributesBuffer = nullptr;
        Buffer* SceneDataBuffer = nullptr;
        Buffer* BuffersIndicesBuffer = nullptr;
        WArray<IndirectCommand> IndirectDrawIndexedArgsArray;
        GBufferRootConstants RootConstants;
        
    public:
        GBufferSystem(ECSManager* eCSManager) : DrawSystem(eCSManager)
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            DummyTexture = Renderer::CreateTexture("DummyTexture", 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data); 
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            if(Manager->EntitiesWith<MeshComponent, Transform>().Count() <= 0)
                return;
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            
            //GBuffer pass
            WArray<MaterialShaderAttribute> materialAttributes;
            
            WArray<Matrix4> worldTransforms;

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

                IndirectCommand command;
                command.DrawId = meshID;
                command.DrawIndexed = {
                    indexCountPerInstance,
                    1,
                    startIndexLocation,
                    baseVertexLocation,
                    0
                };
                
                IndirectDrawIndexedArgsArray.Add(command);

                startIndexLocation += indexCountPerInstance;
                baseVertexLocation += (int)meshComponent.Mesh->VertexData.Num();
                meshID++;
                
                if(!meshComponent.Mesh->CurrentMaterial->HasDiffuseTexture())
                {
                    materialAttributes.Add(MaterialShaderAttribute()); //by default all texture indices = -1, so we can just add empty MaterialAttribute
                    continue;
                }

                int diffuseId = -1, normalId = -1, ormId = -1, clearCoat = -1;
                
                diffuseId = meshComponent.Mesh->CurrentMaterial->GetDiffuseTexture()->GetIndex(SRV_UAV_CBV);
                
                if(meshComponent.Mesh->CurrentMaterial->HasNormalTexture())
                    normalId = meshComponent.Mesh->CurrentMaterial->GetNormalTexture()->GetIndex(SRV_UAV_CBV);
                if(meshComponent.Mesh->CurrentMaterial->HasORMTexture())
                    ormId = meshComponent.Mesh->CurrentMaterial->GetORMTexture()->GetIndex(SRV_UAV_CBV);

                materialAttributes.Add(MaterialShaderAttribute{ diffuseId, normalId, ormId, clearCoat, meshComponent.Mesh->CurrentMaterial->Albedo, meshComponent.Mesh->CurrentMaterial->Metallic, meshComponent.Mesh->CurrentMaterial->Roughness });
            }

            IndirectBuffer = ResourceManager::CreateBuffer("IndirectDrawBuffer", BufferType::IndirectBuffer, IndirectDrawIndexedArgsArray.GetData(), IndirectDrawIndexedArgsArray.GetSize(), sizeof(IndirectCommand));
            VertexBuffer = ResourceManager::CreateBuffer("VertexBuffer", BufferType::VertexBuffer, vertices.GetData(), vertices.GetSize(), sizeof(Vertex));
            IndexBuffer = ResourceManager::CreateBuffer("IndexBuffer", BufferType::IndexBuffer, indices.GetData(), indices.GetSize(), sizeof(uint));
            WorldTransformsBuffer = ResourceManager::CreateBuffer("WorldTransformsBuffer", StorageBuffer, worldTransforms.GetData(), worldTransforms.GetSize(), sizeof(Matrix4));
            MaterialAttributesBuffer = ResourceManager::CreateBuffer("MaterialAttributesBuffer", StorageBuffer, materialAttributes.GetData(), materialAttributes.GetSize(), sizeof(MaterialShaderAttribute));
            SceneDataBuffer = ResourceManager::CreateBuffer("SceneDataBuffer", StorageBuffer, nullptr, sizeof(GBufferSceneData), sizeof(GBufferSceneData));
            GBufferBuffers buffersData { WorldTransformsBuffer->GetIndex(SRV_UAV_CBV), MaterialAttributesBuffer->GetIndex(SRV_UAV_CBV) };
            BuffersIndicesBuffer = ResourceManager::CreateBuffer("BuffersIndicesBuffer", StorageBuffer, &buffersData, sizeof(GBufferBuffers), sizeof(GBufferBuffers));

            RootConstants.BuffersIndicesBuffer = BuffersIndicesBuffer->GetIndex(SRV_UAV_CBV);
            RootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_UAV_CBV);
            
            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            ColorRT = resourceManager->GetRenderTarget("ColorRT");
            ORMRT = resourceManager->GetRenderTarget("ORMRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT = resourceManager->GetRenderTarget("DepthRT");
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            GBufferPipeline = Renderer::CreateGraphicPipeline("GBufferPipeline",
                                                            GBufferPixelShader,
                                                            { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), ORMRT->GetFormat(), MeshIDRT->GetFormat() },
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(GBufferPixelShader) GBufferPixelShader->Destroy();
            if(GBufferPipeline) GBufferPipeline->Destroy();
            if(IndirectBuffer) Renderer::Destroy(IndirectBuffer);
            if(VertexBuffer) Renderer::Destroy(VertexBuffer);
            if(IndexBuffer) Renderer::Destroy(IndexBuffer);

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
            Matrix4 matrices[4];
            for (auto [entity, camera, mainCamera, cameraTransform] : Manager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                matrices[0] = camera.ViewMatrix;
                matrices[1] = camera.ProjectionMatrix;
                matrices[2] = cameraTransform.Matrix;
                matrices[3] = inverse(camera.ProjectionMatrix);
                Renderer::UploadBuffer(SceneDataBuffer, matrices, sizeof(matrices));

                break;
            }
            WArray<Matrix4> worldTransforms;
            for (auto [entity, mesh, transform] : Manager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
            }

            Renderer::UploadBuffer(WorldTransformsBuffer, worldTransforms.GetData(), worldTransforms.GetSize());
            Renderer::SetPipeline(GBufferPipeline);
            Renderer::PushConstants(&RootConstants, sizeof(GBufferRootConstants));
            Renderer::SetRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT }, DepthRT);
            Renderer::ClearRenderTarget(WorldPositionRT);
            Renderer::ClearRenderTarget(NormalRT);
            Renderer::ClearRenderTarget(ColorRT);
            Renderer::ClearRenderTarget(ORMRT);
            Renderer::ClearRenderTarget(MeshIDRT);
            Renderer::ClearDepthStencil(DepthRT);
            
            Renderer::SetVertexBuffers(VertexBuffer, 1);
            Renderer::SetIndexBuffer(IndexBuffer);
            Renderer::DrawIndirect(IndirectDrawIndexedArgsArray.Num(), IndirectBuffer);
            
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
