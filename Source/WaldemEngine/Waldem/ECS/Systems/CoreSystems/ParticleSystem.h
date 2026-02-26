#pragma once
#include "Waldem/Time.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/ParticleSystemComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/ResizableBuffer.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Types/RangeFreeList.h"

namespace Waldem
{
    struct ParticleSystemSceneData
    {
        Matrix4 View;
        Matrix4 Projection;
    };
    
    struct ParticleSystemRootConstants
    {
        uint WorldTransformsBufferID;
        uint SceneBufferID;
        float DeltaTime;
        uint ParticleBuffersIndicesBufferID;
        Vector4 Color;
        Vector3 Size;
        uint ParticlesAmount;
        Vector3 Acceleration;
        float Lifetime;
        uint BufferId;
    };

    struct Particle
    {
        Vector4 Color;
        Vector3 BasePosition;
        float Padding1;
        Vector3 Position;
        float Lifetime;
        Vector3 Velocity;
        float Age;
    };
    
    class WALDEM_API ParticleSystem : public ICoreSystem
    {
        //Post process pass
        Pipeline* ParticleSystemPipeline = nullptr;
        ComputeShader* ParticleSystemComputeShader = nullptr;
        Point3 NumThreads;
        ParticleSystemRootConstants RootConstants;
        Pipeline* ParticleSystemGraphicPipeline = nullptr;
        PixelShader* ParticleSystemPixelShader = nullptr;
        ResizableBuffer IndirectBuffer;
        WArray<IndirectIndexedCommand> IndirectCommands;
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableBuffer WorldTransformsBuffer;
        WArray<Vertex> Vertices;
        WArray<uint> Indices;
        WMap<uint, ResizableBuffer> ParticleBuffersMap;
        ParticleSystemSceneData SceneData;
        Buffer* SceneDataBuffer = nullptr;
        ResizableBuffer ParticleBuffersIndicesBuffer;
        
    public:
        ParticleSystem() {}
        
        void Initialize() override
        {
            ParticleSystemComputeShader = Renderer::LoadComputeShader("ParticleSystem");
            ParticleSystemPipeline = Renderer::CreateComputePipeline("ParticleSystemPipeline", ParticleSystemComputeShader);
            NumThreads = Renderer::GetNumThreadsPerGroup(ParticleSystemComputeShader);
            
            ParticleSystemPixelShader = Renderer::LoadPixelShader("ParticleSystem");
            DepthStencilDesc depthDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthDesc.DepthWriteMask = WD_DEPTH_WRITE_MASK_ZERO;
            depthDesc.DepthFunc = WD_COMPARISON_FUNC_LESS_EQUAL;
            ParticleSystemGraphicPipeline = Renderer::CreateGraphicPipeline("ParticleSystemGraphicPipeline",
                                                            ParticleSystemPixelShader,
                                                            { SGBuffer::GetFormat(Deferred) },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            depthDesc,
                                                            ALPHA_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            VertexBuffer = ResizableBuffer("ParticleVertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), sizeof(Vertex));
            IndexBuffer = ResizableBuffer("ParticleIndexBuffer", BufferType::IndexBuffer, sizeof(uint), sizeof(uint));
            WorldTransformsBuffer = ResizableBuffer("ParticlesTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), 1000);
            IndirectBuffer = ResizableBuffer("ParticleIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectIndexedCommand), 1000);
            SceneDataBuffer = Renderer::CreateBuffer("ParticleSceneDataBuffer", BufferType::StorageBuffer, sizeof(ParticleSystemSceneData), sizeof(ParticleSystemSceneData), &SceneData);
            ParticleBuffersIndicesBuffer = ResizableBuffer("ParticleBuffersIndicesBuffer", BufferType::StorageBuffer, sizeof(uint), 20);
            
            Vertices =
            {
                { {Vector4(-0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(0,1)} },
                { {Vector4( 0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(1,1)} },
                { {Vector4( 0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(1,0)} },
                { {Vector4(-0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(0,0)} },
            };
            Indices = { 0,2,1, 0,3,2 };
            VertexBuffer.AddData(Vertices.GetData(), Vertices.GetSize());
            IndexBuffer.AddData(Indices.GetData(), Indices.GetSize());
            
            ECS::World.observer<ParticleSystemComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, ParticleSystemComponent& particleSystem, Transform& transform)
            {
                auto particleSystemId = IdManager::AddId(entity, ParticleSystemIdType);
                
                if(particleSystemId >= IndirectCommands.Num())
                {
                    IndirectCommands.Add(IndirectIndexedCommand());
                    IndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectIndexedCommand), particleSystemId * sizeof(IndirectIndexedCommand));
                }
                
                ParticleBuffersMap[(uint)particleSystemId] = ResizableBuffer(WString("ParticlesBuffer_") + std::to_string(particleSystemId), BufferType::StorageBuffer, sizeof(Particle), 1000 * sizeof(Particle));
                auto index = ParticleBuffersMap[(uint)particleSystemId].GetIndex(SRV_CBV);
                ParticleBuffersIndicesBuffer.UpdateOrAdd(&index, sizeof(uint), particleSystemId * sizeof(uint));
                WorldTransformsBuffer.UpdateOrAdd(&transform.Matrix, sizeof(Matrix4), particleSystemId * sizeof(Matrix4));
            });

            ECS::World.observer<ParticleSystemComponent>().event(flecs::OnSet).each([&](flecs::entity entity, ParticleSystemComponent& particleSystem)
            {
                int particleSystemId;

                if(IdManager::GetId(entity, ParticleSystemIdType, particleSystemId))
                {
                    auto& command = IndirectCommands[particleSystemId];

                    if(particleSystem.ParticlesAmount != command.Command.InstanceCount)
                    {
                        command.DrawId = particleSystemId;
                        command.Command = {
                            (uint)Indices.Num(),
                            particleSystem.ParticlesAmount,
                            0,
                            0,
                            (uint)particleSystemId
                        };

                        IndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * particleSystemId);
                    }

                    auto& buffer = ParticleBuffersMap[(uint)particleSystemId];
                    if(particleSystem.BufferId != buffer.GetIndex(UAV))
                    {
                        particleSystem.BufferId = buffer.GetIndex(UAV);
                    }

                    if(buffer.Size != sizeof(Particle) * particleSystem.ParticlesAmount)
                    {
                        //TODO: change to resize
                        buffer.UpdateOrAdd(nullptr, sizeof(Particle) * particleSystem.ParticlesAmount, 0);
                    }
                }
            });

            ECS::World.observer<ParticleSystemComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, ParticleSystemComponent& particleSystem)
            {
                int particleSystemId;

                if(IdManager::GetId(entity, ParticleSystemIdType, particleSystemId))
                {
                    IndirectIndexedCommand& command = IndirectCommands[particleSystemId];
                    
                    IndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), particleSystemId * sizeof(IndirectIndexedCommand));
                    
                    command.DrawId = -1;
                    command.Command = { 0, 0, 0, 0, 0 };
                    
                    if(entity.has<Transform>())
                    {
                        WorldTransformsBuffer.RemoveData(sizeof(Matrix4), particleSystemId * sizeof(Matrix4));
                    }

                    Renderer::Destroy(ParticleBuffersMap[(uint)particleSystemId]);
                    ParticleBuffersMap.Remove((uint)particleSystemId);
                    ParticleBuffersIndicesBuffer.RemoveData(sizeof(uint), particleSystemId * sizeof(uint));

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });

            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                if(entity.has<ParticleSystemComponent>())
                {
                    int particleSystemId;

                    if(IdManager::GetId(entity, ParticleSystemIdType, particleSystemId))
                    {
                        WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), particleSystemId * sizeof(Matrix4));
                    }
                }
            });

            ECS::World.system<ParticleSystemComponent, Transform>().kind<ECS::OnDraw>().each([&](flecs::entity entity, ParticleSystemComponent& particleSystem, Transform& transform)
            {
                int particleSystemId;

                if(IdManager::GetId(entity, ParticleSystemIdType, particleSystemId))
                {
                    RootConstants.WorldTransformsBufferID = WorldTransformsBuffer.GetIndex(SRV_CBV);
                    RootConstants.DeltaTime = Time::DeltaTime;
                    RootConstants.Color = particleSystem.Color;
                    RootConstants.Size = particleSystem.Size;
                    RootConstants.ParticlesAmount = particleSystem.ParticlesAmount;
                    RootConstants.Acceleration = particleSystem.Acceleration;
                    RootConstants.Lifetime = particleSystem.Lifetime;
                    RootConstants.BufferId = particleSystem.BufferId;
                    Renderer::SetPipeline(ParticleSystemPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(ParticleSystemRootConstants));
                    Point3 GroupCount = Point3((particleSystem.ParticlesAmount + NumThreads.x - 1) / NumThreads.x, 1, 1);
                    Renderer::Compute(GroupCount);
                }
            });
            
            ECS::World.system().kind<ECS::OnDraw>().each([&]
            {
                if(IndirectCommands.Num() > 0)
                {
                    auto viewport = Renderer::GetCurrentViewport();

                    ECS::Entity linkedCamera;
                    
                    if(viewport->TryGetLinkedCamera(linkedCamera))
                    {
                        auto gbuffer = viewport->GetGBuffer();
                        auto deferred = gbuffer->GetRenderTarget(Deferred);
                        auto depth = gbuffer->GetRenderTarget(Depth);
                        RootConstants.ParticleBuffersIndicesBufferID = ParticleBuffersIndicesBuffer.GetIndex(SRV_CBV);
                        RootConstants.WorldTransformsBufferID = WorldTransformsBuffer.GetIndex(SRV_CBV);
                        RootConstants.SceneBufferID = SceneDataBuffer->GetIndex(SRV_CBV);
                        auto& camera = linkedCamera.get<Camera>();
                        auto& transform = linkedCamera.get<Transform>();
                        SceneData.View = inverse(transform.Matrix);
                        SceneData.Projection = camera.ProjectionMatrix;
                        Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(ParticleSystemSceneData));
                        Renderer::ResourceBarrier(deferred, RENDER_TARGET);
                        Renderer::ResourceBarrier(depth, DEPTH_WRITE);
                        Renderer::SetPipeline(ParticleSystemGraphicPipeline);
                        Renderer::PushConstants(&RootConstants, sizeof(ParticleSystemRootConstants));
                        Renderer::BindRenderTargets({ deferred });
                        Renderer::BindDepthStencil(depth);
                        Renderer::SetVertexBuffers(VertexBuffer.GetBuffer(), 1);
                        Renderer::SetIndexBuffer(IndexBuffer.GetBuffer());
                        Renderer::DrawIndirect(IndirectCommands.Num(), IndirectBuffer);
                        Renderer::ResourceBarrier(deferred, ALL_SHADER_RESOURCE);
                        Renderer::ResourceBarrier(depth, ALL_SHADER_RESOURCE);
                    }
                }
            });
        }
    };
}
