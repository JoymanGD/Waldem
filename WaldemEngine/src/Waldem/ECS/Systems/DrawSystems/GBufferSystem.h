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
#include "Waldem/Renderer/ResizableBuffer.h"

#define MAX_INDIRECT_COMMANDS 500

namespace Waldem
{
    struct GBufferSceneData
    {
        Matrix4 ViewMatrix;
        Matrix4 ProjectionMatrix;
        Matrix4 WorldMatrix;
        Matrix4 InverseProjectionMatrix;
    };

    struct GBufferRootConstants
    {
        uint WorldTransforms;
        uint MaterialAttributes;
        uint SceneDataBuffer;
    };
    
    class WALDEM_API GBufferSystem : public DrawSystem
    {
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
        ResizableBuffer IndirectBuffer;
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer MaterialAttributesBuffer;
        Buffer* SceneDataBuffer = nullptr;
        GBufferRootConstants RootConstants;
        bool CameraIsDirty = true;
        GBufferSceneData SceneData;
        
    public:
        GBufferSystem()
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            DummyTexture = Renderer::CreateTexture("DummyTexture", 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data); 
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            IndirectBuffer = ResizableBuffer("IndirectDrawBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), MAX_INDIRECT_COMMANDS);
            VertexBuffer = ResizableBuffer("VertexBuffer", BufferType::VertexBuffer, sizeof(Vertex));
            IndexBuffer = ResizableBuffer("IndexBuffer", BufferType::IndexBuffer, sizeof(uint));
            WorldTransformsBuffer = ResizableBuffer("WorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), MAX_INDIRECT_COMMANDS);
            MaterialAttributesBuffer = ResizableBuffer("MaterialAttributesBuffer", BufferType::StorageBuffer, sizeof(MaterialShaderAttribute), MAX_INDIRECT_COMMANDS);
            SceneDataBuffer = ResourceManager::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(GBufferSceneData), sizeof(GBufferSceneData));
            
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
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);
            
            ECS::World.observer<MeshComponent, Transform>().event(flecs::OnSet).each([&](flecs::entity entity, MeshComponent& meshComponent, Transform& transform)
            {
                IndirectCommand command;
                command.DrawId = WorldTransformsBuffer.Num();
                command.DrawIndexed = {
                    (uint)meshComponent.Mesh->IndexData.Num(),
                    1,
                    IndexBuffer.Num(),
                    (int)VertexBuffer.Num(),
                    0
                };

                meshComponent.DrawId = command.DrawId;

                Editor::AddEntityID(command.DrawId, entity.id());
                
                VertexBuffer.AddData(meshComponent.Mesh->VertexData.GetData(), meshComponent.Mesh->VertexData.GetSize());
                IndexBuffer.AddData(meshComponent.Mesh->IndexData.GetData(), meshComponent.Mesh->IndexData.GetSize());
                WorldTransformsBuffer.AddData(&transform.Matrix, sizeof(Matrix4));

                IndirectBuffer.AddData(&command, sizeof(IndirectCommand));
                
                auto materialAttribute = MaterialShaderAttribute();

                materialAttribute.Albedo = meshComponent.Mesh->CurrentMaterial->Albedo;
                materialAttribute.Metallic = meshComponent.Mesh->CurrentMaterial->Metallic;
                materialAttribute.Roughness = meshComponent.Mesh->CurrentMaterial->Roughness;
                
                if(meshComponent.Mesh->CurrentMaterial->HasDiffuseTexture())
                {
                    materialAttribute.DiffuseTextureID = meshComponent.Mesh->CurrentMaterial->GetDiffuseTexture()->GetIndex(SRV_UAV_CBV);
                    
                    if(meshComponent.Mesh->CurrentMaterial->HasNormalTexture())
                        materialAttribute.NormalTextureID = meshComponent.Mesh->CurrentMaterial->GetNormalTexture()->GetIndex(SRV_UAV_CBV);
                    if(meshComponent.Mesh->CurrentMaterial->HasORMTexture())
                        materialAttribute.ORMTextureID = meshComponent.Mesh->CurrentMaterial->GetORMTexture()->GetIndex(SRV_UAV_CBV);
                }

                MaterialAttributesBuffer.AddData(&materialAttribute, sizeof(MaterialShaderAttribute));
            });
            
            ECS::World.observer<Transform, MeshComponent>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform, MeshComponent& meshComponent)
            {
                WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), meshComponent.DrawId);
            });

            ECS::World.query<Camera, Transform, EditorComponent>("SceneDataInitializationSystem").each([&](Camera& camera, Transform& transform, EditorComponent)
            {
                SceneData.ViewMatrix = camera.ViewMatrix;
                SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                SceneData.WorldMatrix = transform.Matrix;
                SceneData.InverseProjectionMatrix = glm::inverse(camera.ProjectionMatrix);
                
                CameraIsDirty = true;
            });

            ECS::World.observer<Camera, Transform>("SceneDataUpdateSystem").event(flecs::OnSet).each([&](Camera& camera, Transform& transform)
            {
                SceneData.ViewMatrix = camera.ViewMatrix;
                SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                SceneData.WorldMatrix = transform.Matrix;
                SceneData.InverseProjectionMatrix = glm::inverse(camera.ProjectionMatrix);
                
                CameraIsDirty = true;
            });
            
            ECS::World.system("GBufferSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                if(IndirectBuffer.Num() == 0)
                {
                    return;
                }
                
                if(CameraIsDirty)
                {
                    Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(GBufferSceneData));
                    CameraIsDirty = false;
                }

                RootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_UAV_CBV);
                RootConstants.MaterialAttributes = MaterialAttributesBuffer.GetIndex(SRV_UAV_CBV);
                RootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_UAV_CBV);

                Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ORMRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                
                Renderer::ClearRenderTarget(WorldPositionRT);
                Renderer::ClearRenderTarget(NormalRT);
                Renderer::ClearRenderTarget(ColorRT);
                Renderer::ClearRenderTarget(ORMRT);
                Renderer::ClearRenderTarget(MeshIDRT);
                Renderer::ClearDepthStencil(DepthRT);

                Renderer::SetPipeline(GBufferPipeline);
                Renderer::PushConstants(&RootConstants, sizeof(GBufferRootConstants));
                Renderer::BindRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT });
                Renderer::BindDepthStencil(DepthRT);
                Renderer::SetVertexBuffers(VertexBuffer, 1);
                Renderer::SetIndexBuffer(IndexBuffer);
                Renderer::DrawIndirect(IndirectBuffer.Num(), IndirectBuffer);
                
                Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            });
        }

        void Update(float deltaTime) override
        {
        }

        void OnResize(Vector2 size) override
        {
        }
    };
}
