#pragma once
#include <FlecsUtils.h>

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Components/Sprite.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/ResizableBuffer.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct SpriteRootConstants
    {
        uint WorldTransforms;
        uint TextureIds;
        uint SceneDataBuffer;
    };

    struct SpriteVertex
    {
        Vector3 Position;
        Vector2 TexCoord;
        Vector4 Color;
    };
    struct SpriteSceneData
    {
        Matrix4 ViewMatrix;
        Matrix4 ProjectionMatrix;
        Matrix4 WorldMatrix;
        Matrix4 InverseProjectionMatrix;
    };
    
    class WALDEM_API SpriteRenderingSystem : public ISystem
    {
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        SpriteRootConstants RootConstants;
        SpriteSceneData SceneData;
        Pipeline* SpritePipeline = nullptr;
        PixelShader* SpritePixelShader = nullptr;
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableBuffer IndirectBuffer;
        ResizableBuffer TextureIdsBuffer;
        ResizableBuffer WorldTransformsBuffer;
        Buffer* SceneDataBuffer = nullptr;
        WArray<IndirectCommand> IndirectCommands;
        bool CameraIsDirty = true;
        size_t VerticesCount = 0;
        size_t IndicesCount = 0;
        WArray<SpriteVertex> QuadVertices;
        WArray<uint32> QuadIndices;
        
    public:
        SpriteRenderingSystem() {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {            
            VertexBuffer = ResizableBuffer("SpriteVertexBuffer", BufferType::VertexBuffer, sizeof(SpriteVertex), 1000);
            IndexBuffer  = ResizableBuffer("SpriteIndexBuffer", BufferType::IndexBuffer, sizeof(uint32_t), 1000);
            IndirectBuffer = ResizableBuffer("SpriteIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), 1000);
            TextureIdsBuffer = ResizableBuffer("SpriteTextureIdsBuffer", BufferType::StorageBuffer, sizeof(int), 1000);
            WorldTransformsBuffer = ResizableBuffer("SpriteWorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), 1000);
            SceneDataBuffer = resourceManager->CreateBuffer("SpriteSceneDataBuffer", BufferType::StorageBuffer, sizeof(SpriteSceneData), sizeof(SpriteSceneData), &SceneData);

            QuadVertices =
            {
                { {-0.5f, -0.5f, 0}, {0,1}, {1,1,1,1} }, // bottom-left → uv.y=1
                { { 0.5f, -0.5f, 0}, {1,1}, {1,1,1,1} }, // bottom-right → uv.y=1
                { { 0.5f,  0.5f, 0}, {1,0}, {1,1,1,1} }, // top-right → uv.y=0
                { {-0.5f,  0.5f, 0}, {0,0}, {1,1,1,1} }, // top-left → uv.y=0
            };
            QuadIndices = { 0,1,2, 2,3,0 };
            
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 20, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            ColorRT = resourceManager->GetRenderTarget("ColorRT");
            ORMRT = resourceManager->GetRenderTarget("ORMRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT = resourceManager->GetRenderTarget("DepthRT");

            RasterizerDesc rasterizerDesc = { WD_FILL_MODE_SOLID, WD_CULL_MODE_NONE, false, 0, 0.0f, 0.0f, true, false, false, 0, ConservativeRasterizationMode::WD_CONSERVATIVE_RASTERIZATION_MODE_OFF };
            
            SpritePixelShader = Renderer::LoadPixelShader("Sprite");
            SpritePipeline = Renderer::CreateGraphicPipeline("SpritePipeline",
                                                             SpritePixelShader,
                                                             { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), ORMRT->GetFormat(), MeshIDRT->GetFormat() },
                                                             TextureFormat::D32_FLOAT,
                                                             rasterizerDesc,
                                                             DEFAULT_DEPTH_STENCIL_DESC,
                                                             WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                             inputElementDescs);

            ECS::World.query<Camera, Transform>("SceneDataInitializationSystem").each([&](Camera& camera, Transform& transform)
            {
                SceneData.ViewMatrix = camera.ViewMatrix;
                SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                SceneData.WorldMatrix = transform.Matrix;
                SceneData.InverseProjectionMatrix = inverse(camera.ProjectionMatrix);
                
                CameraIsDirty = true;
            });

            ECS::World.observer<Camera, Transform>("SpriteSceneDataUpdateSystem").event(flecs::OnSet).each([&](flecs::entity entity, Camera& camera, Transform& transform)
            {
                SceneData.ViewMatrix = camera.ViewMatrix;
                SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                SceneData.WorldMatrix = transform.Matrix;
                SceneData.InverseProjectionMatrix = inverse(camera.ProjectionMatrix);
                
                CameraIsDirty = true;
            });

            ECS::World.observer<Sprite, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Sprite& sprite, Transform& transform)
            {
                auto drawId = IdManager::AddId(entity, DrawIdType);
                
                if(drawId >= IndirectCommands.Num())
                {
                    IndirectCommand command;
                    command.DrawId = drawId;
                    command.DrawIndexed = {
                        6,
                        1,
                        (uint)IndicesCount,
                        (int)VerticesCount,
                        0
                    };
                    IndirectCommands.Add(command);
                    IndirectBuffer.AddData(&command, sizeof(IndirectCommand));
                    WorldTransformsBuffer.AddData(&transform.Matrix, sizeof(Matrix4));

                    VertexBuffer.AddData(QuadVertices.GetData(), QuadVertices.GetSize());
                    IndexBuffer.AddData(QuadIndices.GetData(), QuadIndices.GetSize());
                    
                    VerticesCount += 4;
                    IndicesCount += 6;
                }
                else
                {
                    IndirectCommand& command = IndirectCommands[drawId];
                    command.DrawId = drawId;
                    command.DrawIndexed = {
                        6,
                        1,
                        (uint)IndicesCount,
                        (int)VerticesCount,
                        0
                    };
                    IndirectBuffer.UpdateData(&command, sizeof(IndirectCommand), drawId * sizeof(IndirectCommand));
                    WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), drawId * sizeof(IndirectCommand));
                    
                    VertexBuffer.UpdateData(QuadVertices.GetData(), QuadVertices.GetSize(), drawId * sizeof(SpriteVertex) * 4);
                    IndexBuffer.UpdateData(QuadIndices.GetData(), QuadIndices.GetSize(), drawId * sizeof(uint32) * 6);
                    
                    VerticesCount += 4;
                    IndicesCount += 6;
                }
            });
            
            ECS::World.observer<Sprite>().event(flecs::OnSet).each([&](flecs::entity entity, Sprite& sprite)
            {
                int drawId;

                if(IdManager::GetId(entity, DrawIdType, drawId))
                {
                    bool textureReferenceIsEmpty = sprite.TextureRef.Reference.empty() || sprite.TextureRef.Reference == "Empty";
                    
                    if(textureReferenceIsEmpty && !sprite.TextureRef.IsValid())
                    {
                        return;
                    }

                    if(!textureReferenceIsEmpty && !sprite.TextureRef.IsValid())
                    {
                        sprite.TextureRef.LoadAsset(contentManager);
                    }
                    
                    if(sprite.TextureRef.IsValid())
                    {
                        int index = sprite.TextureRef.Texture->GetIndex(SRV_UAV_CBV);
                        TextureIdsBuffer.AddData(&index, sizeof(int));
                    }
                }
            });
            
            ECS::World.observer<Sprite>().event(flecs::OnRemove).each([&](flecs::entity entity, Sprite& sprite)
            {
                int drawId;

                if(IdManager::GetId(entity, DrawIdType, drawId))
                {
                    IndirectCommand& command = IndirectCommands[drawId];
                    IndirectBuffer.RemoveData(sizeof(IndirectCommand), drawId * sizeof(IndirectCommand));
                    command.DrawId = -1;
                    command.DrawIndexed = { 0, 0, 0, 0, 0 };

                    if(entity.has<Transform>())
                    {
                        WorldTransformsBuffer.RemoveData(sizeof(Matrix4), drawId * sizeof(Matrix4));
                    }
                    
                    TextureIdsBuffer.RemoveData(sizeof(int), drawId * sizeof(int));

                    IdManager::RemoveId(entity, DrawIdType);
                }
            });
            
            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                if(entity.has<Sprite>())
                {
                    int drawId;

                    if(IdManager::GetId(entity, DrawIdType, drawId))
                    {
                        WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), drawId * sizeof(IndirectCommand));
                    }
                }
            });
            
            ECS::World.system("SpriteRenderingSystem").kind(flecs::OnDraw).each([&]
            {
                if(CameraIsDirty)
                {
                    Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(SpriteSceneData));
                    CameraIsDirty = false;
                }

                RootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_UAV_CBV);
                RootConstants.TextureIds = TextureIdsBuffer.GetIndex(SRV_UAV_CBV);
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

                Renderer::SetPipeline(SpritePipeline);
                Renderer::PushConstants(&RootConstants, sizeof(SpriteRootConstants));
                Renderer::BindRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT });
                Renderer::BindDepthStencil(DepthRT);
                Renderer::SetVertexBuffers(VertexBuffer, 1);
                Renderer::SetIndexBuffer(IndexBuffer);
                Renderer::DrawIndirect(IndirectCommands.Num(), IndirectBuffer);
                
                Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            });
        }
    };
}