#pragma once
#include <unordered_map>
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
        uint Indirection;
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

    struct SpriteMeta
    {
        uint32_t slotIndex;        // slot in WorldTransforms/TextureIds
        uint32_t indirectionIndex; // index in IndirectionBuffer
    };

    class WALDEM_API SpriteRenderingSystem : public ISystem
    {
        // Render targets
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;

        SpriteRootConstants RootConstants{};
        SpriteSceneData SceneData{};

        Pipeline* SpritePipeline = nullptr;
        PixelShader* SpritePixelShader = nullptr;

        // Buffers
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableBuffer IndirectionBuffer;   // maps instanceID → slotIndex
        ResizableBuffer TextureIdsBuffer;
        ResizableBuffer WorldTransformsBuffer;
        Buffer* SceneDataBuffer = nullptr;

        WArray<SpriteVertex> QuadVertices;
        WArray<uint32> QuadIndices;

        size_t VerticesCount = 0;
        size_t IndicesCount = 0;

        bool CameraIsDirty = true;

        // Bookkeeping
        std::unordered_map<uint64, SpriteMeta> SpriteTable;
        std::vector<flecs::entity> IndirectionToEntity; // reverse lookup
        FreeList SlotAllocator;    // manages slot indices
        uint32_t InstanceCount = 0; // number of active sprites

    public:
        SpriteRenderingSystem() {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            VertexBuffer          = ResizableBuffer("SpriteVertexBuffer", BufferType::VertexBuffer, sizeof(SpriteVertex), 1000);
            IndexBuffer           = ResizableBuffer("SpriteIndexBuffer", BufferType::IndexBuffer, sizeof(uint32_t), 1000);
            IndirectionBuffer     = ResizableBuffer("SpriteIndirectionBuffer", BufferType::StorageBuffer, sizeof(uint32_t), 1000);
            TextureIdsBuffer      = ResizableBuffer("SpriteTextureIdsBuffer", BufferType::StorageBuffer, sizeof(int), 1000);
            WorldTransformsBuffer = ResizableBuffer("SpriteWorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), 1000);
            SceneDataBuffer       = resourceManager->CreateBuffer("SpriteSceneDataBuffer", BufferType::StorageBuffer, sizeof(SpriteSceneData), sizeof(SpriteSceneData), &SceneData);

            // Quad geometry
            QuadVertices =
            {
                { {-0.5f, -0.5f, 0}, {0,1}, {1,1,1,1} },
                { { 0.5f, -0.5f, 0}, {1,1}, {1,1,1,1} },
                { { 0.5f,  0.5f, 0}, {1,0}, {1,1,1,1} },
                { {-0.5f,  0.5f, 0}, {0,0}, {1,1,1,1} },
            };
            QuadIndices = { 0,1,2, 2,3,0 };

            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT,    0, 0,  WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT,       0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, TextureFormat::R32G32B32A32_FLOAT, 0, 20, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // Setup pipeline
            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT        = resourceManager->GetRenderTarget("NormalRT");
            ColorRT         = resourceManager->GetRenderTarget("ColorRT");
            ORMRT           = resourceManager->GetRenderTarget("ORMRT");
            MeshIDRT        = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT         = resourceManager->GetRenderTarget("DepthRT");

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

            // OnAdd: allocate slot, write transform, setup indirection
            ECS::World.observer<Sprite, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Sprite& sprite, Transform& transform)
            {
                uint32_t slotIndex = SlotAllocator.Allocate();

                // Write transform now
                WorldTransformsBuffer.UpdateOrAdd(&transform.Matrix, sizeof(Matrix4), slotIndex * sizeof(Matrix4));

                // Append slot index into indirection buffer
                uint32_t indirectionIndex = InstanceCount;
                IndirectionBuffer.UpdateOrAdd(&slotIndex, sizeof(uint32_t), indirectionIndex * sizeof(uint32_t));

                if (indirectionIndex >= IndirectionToEntity.size())
                    IndirectionToEntity.resize(indirectionIndex + 1);

                IndirectionToEntity[indirectionIndex] = entity;

                SpriteTable[entity.id()] = { slotIndex, indirectionIndex };
                InstanceCount++;
            });

            // OnSet: when sprite data (like texture) becomes valid
            ECS::World.observer<Sprite>().event(flecs::OnSet).each([&](flecs::entity entity, Sprite& sprite)
            {
                auto it = SpriteTable.find(entity.id());
                if (it == SpriteTable.end()) return;

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
                    TextureIdsBuffer.UpdateOrAdd(&index, sizeof(int), it->second.slotIndex * sizeof(int));
                }
            });

            ECS::World.observer<Sprite>().event(flecs::OnRemove).each([&](flecs::entity entity, Sprite& sprite)
            {
                auto it = SpriteTable.find(entity.id());
                if (it == SpriteTable.end()) return;

                SpriteMeta meta = it->second;
                uint32_t idxToRemove = meta.indirectionIndex;
                uint32_t lastIdx = InstanceCount - 1;

                if (idxToRemove != lastIdx)
                {
                    // Get swapped entity directly
                    flecs::entity swappedEntity = IndirectionToEntity[lastIdx];
                    SpriteMeta& swappedMeta = SpriteTable[swappedEntity.id()];

                    // Write swapped slot into removed index
                    IndirectionBuffer.UpdateData(&swappedMeta.slotIndex, sizeof(uint32_t), idxToRemove * sizeof(uint32_t));

                    // Update swapped sprite’s indirection index
                    swappedMeta.indirectionIndex = idxToRemove;
                    IndirectionToEntity[idxToRemove] = swappedEntity;
                }

                InstanceCount--;
                SlotAllocator.Free(meta.slotIndex);
                SpriteTable.erase(it);
            });

            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                auto it = SpriteTable.find(entity.id());
                if (it != SpriteTable.end())
                {
                    WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), it->second.slotIndex * sizeof(Matrix4));
                }
            });

            // Rendering
            ECS::World.system("SpriteRenderingSystem").kind(flecs::OnDraw).each([&]
            {
                if(CameraIsDirty)
                {
                    Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(SpriteSceneData));
                    CameraIsDirty = false;
                }

                RootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_UAV_CBV);
                RootConstants.TextureIds      = TextureIdsBuffer.GetIndex(SRV_UAV_CBV);
                RootConstants.Indirection     = IndirectionBuffer.GetIndex(SRV_UAV_CBV);
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

                if (InstanceCount > 0)
                {
                    Renderer::DrawIndexedInstanced(6, InstanceCount, 0, 0, 0);
                }

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
