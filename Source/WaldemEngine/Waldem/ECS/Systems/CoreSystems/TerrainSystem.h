#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Terrain.h"
#include "Waldem/ECS/Components/TerrainMeshComponent.h"
#include "Waldem/Renderer/Model/TerrainMesh.h"

namespace Waldem
{
    #define MAX_TERRAIN_INDIRECT_COMMANDS 10
    
    struct TerrainVertex
    {
        Vector3 Position;
        Vector3 Normal;
        Vector2 UV;
    };

    struct STerrainData
    {
        Matrix4 View;
        Matrix4 Proj;
        Matrix4 InvProj;
        Matrix4 WorldTransform;
        uint HeightMapIndex;
        uint AlbedoIndex;
        int Resolution;
        float Height;
        uint VertexBufferIndex;
        uint IndexBufferIndex;
        uint PhysXSamplesBufferIndex;
    };
    
    class WALDEM_API TerrainSystem : public ISystem
    {
    private:
        Pipeline* TerrainGenerationPipeline = nullptr;
        ComputeShader* TerrainGenerationComputeShader = nullptr;
        Pipeline* TerrainRenderingPipeline = nullptr;
        PixelShader* TerrainPixelShader = nullptr;
        ResizableBuffer IndirectBuffer;
        WArray<IndirectIndexedCommand> IndirectCommands;

        void PopulateTerrain(ECS::Entity entity, Terrain& terrain)
        {
            if(!terrain.Heightmap.IsEmpty() && !terrain.Heightmap.IsValid())
            {
                terrain.Heightmap.LoadAsset();
            }
            
            if(!terrain.Albedo.IsEmpty() && !terrain.Albedo.IsValid())
            {
                terrain.Albedo.LoadAsset();
            }

            auto& transformComponent = entity.get<Transform>();

            struct
            {
                uint HeightMapIndex;
                uint AlbedoIndex;
                int Resolution;
                float Height;
            } terrainData;
            
            terrainData.HeightMapIndex = !terrain.Heightmap.IsEmpty() && terrain.Heightmap.IsValid() ? terrain.Heightmap.Texture->GetIndex(SRV_CBV) : 0;
            terrainData.AlbedoIndex = !terrain.Albedo.IsEmpty() && terrain.Albedo.IsValid() ? terrain.Albedo.Texture->GetIndex(SRV_CBV) : 0;
            terrainData.Resolution = terrain.Resolution;
            terrainData.Height = terrain.Height;
            
            Renderer::UploadBuffer(terrain.DataBuffer, &terrainData, sizeof(terrainData), sizeof(Matrix4) * 4);
            
            auto sceneDataBufferIndex = terrain.DataBuffer->GetIndex(SRV_CBV);
            
            Renderer::SetPipeline(TerrainGenerationPipeline);
            Renderer::PushConstants(&sceneDataBufferIndex, sizeof(uint));
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(TerrainGenerationComputeShader);
            auto groupCount = Point3((terrain.Resolution + numThreads.x - 1) / numThreads.x, (terrain.Resolution + numThreads.y - 1) / numThreads.y, 1);
            Renderer::Compute(groupCount);
            Renderer::UAVBarrier(terrain.PhysXSamplesBuffer->GetBuffer());

            int globalDrawId;

            if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
            {
                WString name = "Terrain";
                Renderer::RenderData.TLAS.SetData(globalDrawId, name, terrain.VertexBuffer->GetBuffer(), terrain.IndexBuffer->GetBuffer(), terrain.DrawCommand, terrain.VertexCount, transformComponent.RenderMatrix);
            }
        }
    public:
        TerrainSystem() {}

        void Initialize() override
        {
            IndirectBuffer = ResizableBuffer("TerrainIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectIndexedCommand), MAX_TERRAIN_INDIRECT_COMMANDS);

            TerrainGenerationComputeShader = Renderer::LoadComputeShader("Terrain");
            TerrainGenerationPipeline = Renderer::CreateComputePipeline("TerrainGenerationPipeline", TerrainGenerationComputeShader);
            
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, TextureFormat::R32G32B32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 24, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            TerrainPixelShader = Renderer::LoadPixelShader("Terrain");
            TerrainRenderingPipeline = Renderer::CreateGraphicPipeline("TerrainPipeline",
                                                            TerrainPixelShader,
                                                            { SGBuffer::GetFormat(WorldPosition), SGBuffer::GetFormat(Normal), SGBuffer::GetFormat(Color), SGBuffer::GetFormat(ORM), SGBuffer::GetFormat(MeshID) },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);

            Observer<Terrain>("OnTerrainAdd", flecs::OnAdd, [&](ECS::Entity entity, Terrain& terrain)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                terrain.VertexCount = terrain.Resolution * terrain.Resolution;
                terrain.IndexCount = (terrain.Resolution-1) * (terrain.Resolution-1) * 6;
                
                terrain.VertexBuffer = new ResizableBuffer("TerrainVertexBuffer", BufferType::VertexBuffer, sizeof(TerrainVertex), terrain.VertexCount);
                terrain.IndexBuffer = new ResizableBuffer("TerrainIndexBuffer", BufferType::IndexBuffer, sizeof(uint), terrain.IndexCount);
                int samplesCount = terrain.VertexCount;
                terrain.PhysXSamplesBuffer = new ResizableBuffer("PhysXSamplesBuffer", BufferType::StorageBuffer, sizeof(PhysXTerrainSample), samplesCount);
                terrain.DataBuffer = Renderer::CreateBuffer("TerrainDataBuffer", BufferType::StorageBuffer, sizeof(STerrainData), sizeof(STerrainData));

                struct
                {
                    uint VertexBufferIndex;
                    uint IndexBufferIndex;
                    uint PhysXSamplesBufferIndex;
                } terrainData;

                terrainData.VertexBufferIndex = terrain.VertexBuffer->GetIndex(UAV);
                terrainData.IndexBufferIndex = terrain.IndexBuffer->GetIndex(UAV);
                terrainData.PhysXSamplesBufferIndex = terrain.PhysXSamplesBuffer->GetIndex(UAV);
                
                Renderer::UploadBuffer(terrain.DataBuffer, &terrainData, sizeof(terrainData), sizeof(Matrix4) * 4 + sizeof(uint) * 4);

                IndirectIndexedCommand command;
                command.DrawId = 999;
                command.Command = { terrain.IndexCount, 1, 0, 0, 0};
                IndirectCommands.Add(command);
                
                terrain.DrawCommand = command.Command;

                IndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand));

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });
            
            Observer<Terrain>("OnTerrainSet", flecs::OnSet, [&](ECS::Entity entity, Terrain& terrain)
            {
                PopulateTerrain(entity, terrain);

                // Renderer::RenderData.TLAS.UpdateGeometry(entry.GlobalDrawId, Renderer::RenderData.VertexBuffer.GetBuffer(), Renderer::RenderData.IndexBuffer.GetBuffer(), entry.DrawCommand, entry.VertexCount);
            });

            Observer<Transform>("TerrainRenderingTransformSyncOnSet", flecs::OnSet, [&](flecs::entity entity, Transform& transform)
            {
                if(entity.has<Terrain>())
                {
                    int globalDrawId;
                    if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                    {
                        Renderer::RenderData.TLAS.UpdateTransform(globalDrawId, transform);
                    }
                }
            });
            
            System<ECS::OnDraw, Terrain>("TerrainRenderinSystem", [&](ECS::Entity entity, Terrain& terrain)
            {
                auto viewport = Renderer::GetCurrentViewport();
                ECS::Entity linkedCamera;
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& cameraTransformComponent = linkedCamera.get<Transform>();
                    auto& transformComponent = entity.get<Transform>();

                    struct
                    {
                        Matrix4 View;
                        Matrix4 Proj;
                        Matrix4 InvProj;
                        Matrix4 WorldTransform;
                    } terrainData;
                
                    terrainData.Proj = cameraComponent.ProjectionMatrix;
                    terrainData.View = inverse(cameraTransformComponent.RenderMatrix);
                    terrainData.WorldTransform = transformComponent.RenderMatrix;
                    terrainData.InvProj = inverse(cameraComponent.ProjectionMatrix);
                    
                    Renderer::UploadBuffer(terrain.DataBuffer, &terrainData, sizeof(Matrix4) * 4);
                    auto sceneDataBufferIndex = terrain.DataBuffer->GetIndex(SRV_CBV);

                    auto gbuffer = viewport->GetGBuffer();
                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    
                    Renderer::SetPipeline(TerrainRenderingPipeline);
                    Renderer::PushConstants(&sceneDataBufferIndex, sizeof(uint));
                    Renderer::BindRenderTargets({ gbuffer->GetRenderTarget(WorldPosition), gbuffer->GetRenderTarget(Normal), gbuffer->GetRenderTarget(Color), gbuffer->GetRenderTarget(ORM), gbuffer->GetRenderTarget(MeshID) });
                    Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                    Renderer::SetVertexBuffers(terrain.VertexBuffer->GetBuffer(), 1);
                    Renderer::SetIndexBuffer(terrain.IndexBuffer->GetBuffer());
                    Renderer::DrawIndirect(IndirectCommands.Num(), IndirectBuffer);
                    
                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, RENDER_TARGET, ALL_SHADER_RESOURCE);
                    gbuffer->Barrier(Depth, DEPTH_WRITE, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}
