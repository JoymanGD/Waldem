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
        Buffer* DummyVertexBuffer = nullptr;
        Buffer* DummyIndexBuffer = nullptr;
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
        WArray<IndirectCommand> IndirectCommands;
        WArray<MaterialShaderAttribute> MaterialAttributes;
        
    public:
        GBufferSystem()
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            DummyTexture = Renderer::CreateTexture("DummyTexture", 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data);

            WArray DummyVertexData
            {
                Vertex(Vector3(0, 0, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
                Vertex(Vector3(0, 2, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
                Vertex(Vector3(2, 2, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
            };
            WArray DummyIndexData { 0, 1, 2 };
            DummyVertexBuffer = Renderer::CreateBuffer("DummyVertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), sizeof(Vertex));
            DummyIndexBuffer = Renderer::CreateBuffer("DummyIndexBuffer", BufferType::IndexBuffer, sizeof(uint), sizeof(uint));
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

            ECS::World.query<Camera, Transform, EditorComponent>("SceneDataInitializationSystem").each([&](Camera& camera, Transform& transform, EditorComponent)
            {
                SceneData.ViewMatrix = camera.ViewMatrix;
                SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                SceneData.WorldMatrix = transform.Matrix;
                SceneData.InverseProjectionMatrix = glm::inverse(camera.ProjectionMatrix);
                
                CameraIsDirty = true;
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnAdd).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                IndirectCommand command;
                command.DrawId = WorldTransformsBuffer.Num();
                command.DrawIndexed = { 1, 1, 0, 0, 0 };
                IndirectCommands.Add(command);

                meshComponent.DrawId = command.DrawId;
                IndirectBuffer.AddData(&command, sizeof(IndirectCommand));
                
                auto transform = entity.get<Transform>();
                
                if(transform)
                {
                    WorldTransformsBuffer.AddData((Matrix4*)&transform->Matrix, sizeof(Matrix4));
                }

                auto materialAttribute = MaterialShaderAttribute();
                MaterialAttributes.Add(materialAttribute);
                MaterialAttributesBuffer.AddData(&materialAttribute, sizeof(MaterialShaderAttribute));

                Editor::AddEntityID(command.DrawId, entity.id());
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnSet).each([&](MeshComponent& meshComponent)
            {
                if(!meshComponent.MeshRef.Reference.empty())
                {
                    meshComponent.MeshRef.LoadAsset(contentManager);
                }
                
                if(meshComponent.MeshRef.IsValid() && meshComponent.DrawId >= 0)
                {
                    auto& command = IndirectCommands[meshComponent.DrawId];
                    command.DrawIndexed = {
                        (uint)meshComponent.MeshRef.Mesh->IndexData.Num(),
                        1,
                        IndexBuffer.Num(),
                        (int)VertexBuffer.Num(),
                        0
                    };

                    IndirectBuffer.UpdateData(&command, sizeof(IndirectCommand), sizeof(IndirectCommand) * meshComponent.DrawId);

                    VertexBuffer.AddData(meshComponent.MeshRef.Mesh->VertexData.GetData(), meshComponent.MeshRef.Mesh->VertexData.GetSize());
                    IndexBuffer.AddData(meshComponent.MeshRef.Mesh->IndexData.GetData(), meshComponent.MeshRef.Mesh->IndexData.GetSize());

                    auto& materialAttribute = MaterialAttributes[meshComponent.DrawId];

                    materialAttribute.Albedo = meshComponent.MeshRef.Mesh->CurrentMaterial->Albedo;
                    materialAttribute.Metallic = meshComponent.MeshRef.Mesh->CurrentMaterial->Metallic;
                    materialAttribute.Roughness = meshComponent.MeshRef.Mesh->CurrentMaterial->Roughness;
                    
                    if(meshComponent.MeshRef.Mesh->CurrentMaterial->HasDiffuseTexture())
                    {
                        materialAttribute.DiffuseTextureID = meshComponent.MeshRef.Mesh->CurrentMaterial->GetDiffuseTexture()->GetIndex(SRV_UAV_CBV);
                        
                        if(meshComponent.MeshRef.Mesh->CurrentMaterial->HasNormalTexture())
                            materialAttribute.NormalTextureID = meshComponent.MeshRef.Mesh->CurrentMaterial->GetNormalTexture()->GetIndex(SRV_UAV_CBV);
                        if(meshComponent.MeshRef.Mesh->CurrentMaterial->HasORMTexture())
                            materialAttribute.ORMTextureID = meshComponent.MeshRef.Mesh->CurrentMaterial->GetORMTexture()->GetIndex(SRV_UAV_CBV);
                    }

                    MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), sizeof(MaterialShaderAttribute) * meshComponent.DrawId);
                }
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                IndirectCommand& command = IndirectCommands[meshComponent.DrawId];
                command.DrawId = -1;
                command.DrawIndexed = { 0, 0, 0, 0, 0 };

                IndirectBuffer.RemoveData(sizeof(IndirectCommand), meshComponent.DrawId * sizeof(IndirectCommand));
                
                auto transform = entity.get<Transform>();
                
                if(transform)
                {
                    WorldTransformsBuffer.RemoveData(sizeof(Matrix4), meshComponent.DrawId * sizeof(Matrix4));
                }

                MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), meshComponent.DrawId * sizeof(MaterialShaderAttribute));

                Editor::RemoveEntityID(meshComponent.DrawId);
            });
            
            ECS::World.observer<Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Transform& transform)
            {
                auto meshComponent = entity.get<MeshComponent>();
                
                if(meshComponent && meshComponent->IsValid())
                {
                    WorldTransformsBuffer.AddData(&transform.Matrix, sizeof(Matrix4));
                }
            });
            
            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                auto meshComponent = entity.get<MeshComponent>();
                
                if(meshComponent && meshComponent->IsValid() && meshComponent->DrawId >= 0)
                {
                    WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), meshComponent->DrawId * sizeof(Matrix4));
                }
            });

            ECS::World.observer<Camera>("SceneDataUpdateSystem").event(flecs::OnSet).each([&](flecs::entity entity, Camera& camera)
            {
                auto transform = entity.get<Transform>();

                if(transform)
                {
                    SceneData.ViewMatrix = camera.ViewMatrix;
                    SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                    SceneData.WorldMatrix = transform->Matrix;
                    SceneData.InverseProjectionMatrix = glm::inverse(camera.ProjectionMatrix);
                    
                    CameraIsDirty = true;
                }
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
