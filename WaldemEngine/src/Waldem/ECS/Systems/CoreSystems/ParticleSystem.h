#pragma once
#include <FlecsUtils.h>

#include "Waldem/Time.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/ParticleSystemComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/ResizableBuffer.h"
#include "Waldem/Renderer/Shader.h"

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
        uint ParticlesBufferID;
        uint ParticlesSystemDataBufferID;
        float DeltaTime;
    };

    struct Particle
    {
        Vector4 Color;
        Vector3 BasePosition;
        float Padding1;
        Vector3 Position;
        float  Lifetime;
        Vector3 Velocity;
        float  Age;
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
        RenderTarget* TargetRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        ResizableBuffer IndirectBuffer;
        WArray<IndirectCommand> IndirectCommands;
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer ParticlesPositionsBuffer;
        WArray<Vertex> Vertices;
        WArray<uint> Indices;
        WArray<Particle> Particles;
        ResizableBuffer ParticlesBuffer;
        ParticleSystemSceneData SceneData;
        Buffer* SceneDataBuffer = nullptr;
        Buffer* ParticlesSystemDataBuffer = nullptr;
        
    public:
        ParticleSystem() {}
        
        void Initialize() override
        {
            TargetRT = Renderer::GetRenderTarget("TargetRT");
            DepthRT = Renderer::GetRenderTarget("DepthRT");
            
            ParticleSystemComputeShader = Renderer::LoadComputeShader("ParticleSystem");
            ParticleSystemPipeline = Renderer::CreateComputePipeline("ParticleSystemPipeline", ParticleSystemComputeShader);
            NumThreads = Renderer::GetNumThreadsPerGroup(ParticleSystemComputeShader);
            
            ParticleSystemPixelShader = Renderer::LoadPixelShader("ParticleSystem");
            DepthStencilDesc depthDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthDesc.DepthWriteMask = WD_DEPTH_WRITE_MASK_ZERO;
            depthDesc.DepthFunc = WD_COMPARISON_FUNC_LESS_EQUAL;
            ParticleSystemGraphicPipeline = Renderer::CreateGraphicPipeline("ParticleSystemGraphicPipeline",
                                                            ParticleSystemPixelShader,
                                                            { TargetRT->GetFormat() },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            depthDesc,
                                                            ALPHA_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            VertexBuffer = ResizableBuffer("ParticleVertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), sizeof(Vertex));
            IndexBuffer = ResizableBuffer("ParticleIndexBuffer", BufferType::IndexBuffer, sizeof(uint), sizeof(uint));
            ParticlesBuffer = ResizableBuffer("ParticlesBuffer", BufferType::StorageBuffer, sizeof(Particle), 1000);
            WorldTransformsBuffer = ResizableBuffer("ParticlesTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), 1000);
            IndirectBuffer = ResizableBuffer("ParticleIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), 1000);
            SceneDataBuffer = Renderer::CreateBuffer("ParticleSceneDataBuffer", BufferType::StorageBuffer, sizeof(ParticleSystemSceneData), sizeof(ParticleSystemSceneData), &SceneData);
            ParticlesSystemDataBuffer = Renderer::CreateBuffer("ParticlesSystemDataBuffer", BufferType::StorageBuffer, sizeof(ParticleSystemComponent), sizeof(ParticleSystemComponent), nullptr);
            
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
                    IndirectCommands.Add(IndirectCommand());
                    IndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectCommand), particleSystemId * sizeof(IndirectCommand));
                }
                
                WorldTransformsBuffer.UpdateOrAdd(&transform.Matrix, sizeof(Matrix4), particleSystemId * sizeof(Matrix4));
            });

            ECS::World.observer<ParticleSystemComponent>().event(flecs::OnSet).each([&](flecs::entity entity, ParticleSystemComponent& particleSystem)
            {
                int particleSystemId;

                if(IdManager::GetId(entity, ParticleSystemIdType, particleSystemId))
                {
                    auto& command = IndirectCommands[particleSystemId];

                    if(particleSystem.ParticlesAmount != command.DrawIndexed.InstanceCount)
                    {
                        command.DrawId = particleSystemId;
                        command.DrawIndexed = {
                            (uint)Indices.Num(),
                            particleSystem.ParticlesAmount,
                            0,
                            0,
                            (uint)particleSystemId
                        };

                        IndirectBuffer.UpdateData(&command, sizeof(IndirectCommand), sizeof(IndirectCommand) * particleSystemId);
                    }

                    ParticlesBuffer.UpdateOrAdd(nullptr, sizeof(Particle) * particleSystem.ParticlesAmount, sizeof(Particle) * particleSystemId);
                }
            });

            ECS::World.observer<ParticleSystemComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, ParticleSystemComponent& particleSystem)
            {
                int particleSystemId;

                if(IdManager::GetId(entity, ParticleSystemIdType, particleSystemId))
                {
                    IndirectCommand& command = IndirectCommands[particleSystemId];
                    
                    IndirectBuffer.RemoveData(sizeof(IndirectCommand), particleSystemId * sizeof(IndirectCommand));
                    
                    command.DrawId = -1;
                    command.DrawIndexed = { 0, 0, 0, 0, 0 };
                    
                    auto transform = entity.get<Transform>();
                    
                    if(transform)
                    {
                        WorldTransformsBuffer.RemoveData(sizeof(Matrix4), particleSystemId * sizeof(Matrix4));
                    }

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

            ECS::World.system<ParticleSystemComponent, Transform>().kind(flecs::OnDraw).each([&](flecs::entity entity, ParticleSystemComponent& particleSystem, Transform& transform)
            {
                RootConstants.ParticlesBufferID = ParticlesBuffer.GetIndex(UAV);
                RootConstants.WorldTransformsBufferID = WorldTransformsBuffer.GetIndex(SRV_CBV);
                RootConstants.ParticlesSystemDataBufferID = ParticlesSystemDataBuffer->GetIndex(SRV_CBV);
                RootConstants.DeltaTime = Time::DeltaTime;
                Renderer::UploadBuffer(ParticlesSystemDataBuffer, &particleSystem, sizeof(ParticleSystemComponent));
                Renderer::SetPipeline(ParticleSystemPipeline);
                Renderer::PushConstants(&RootConstants, sizeof(ParticleSystemRootConstants));
                Point3 GroupCount = Point3((particleSystem.ParticlesAmount + NumThreads.x - 1) / NumThreads.x, 1, 1);
                Renderer::Compute(GroupCount);
                Renderer::UAVBarrier(ParticlesBuffer);
            });
            
            ECS::World.system().kind(flecs::OnDraw).each([&]
            {
                if(IndirectCommands.Num() > 0)
                {
                    auto viewport = Renderer::GetCurrentViewport();

                    ECS::Entity linkedCamera;
                    
                    if(viewport->TryGetLinkedCamera(linkedCamera))
                    {
                        RootConstants.ParticlesBufferID = ParticlesBuffer.GetIndex(SRV_CBV);
                        RootConstants.WorldTransformsBufferID = WorldTransformsBuffer.GetIndex(SRV_CBV);
                        RootConstants.SceneBufferID = SceneDataBuffer->GetIndex(SRV_CBV);
                        auto camera = linkedCamera.get<Camera>();
                        SceneData.View = camera->ViewMatrix;
                        SceneData.Projection = camera->ProjectionMatrix;
                        Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(ParticleSystemSceneData));
                        Renderer::ResourceBarrier(TargetRT, RENDER_TARGET);
                        Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE);
                        Renderer::SetPipeline(ParticleSystemGraphicPipeline);
                        Renderer::PushConstants(&RootConstants, sizeof(ParticleSystemRootConstants));
                        Renderer::BindRenderTargets({ TargetRT });
                        Renderer::BindDepthStencil(DepthRT);
                        Renderer::SetVertexBuffers(VertexBuffer.GetBuffer(), 1);
                        Renderer::SetIndexBuffer(IndexBuffer.GetBuffer());
                        Renderer::DrawIndirect(IndirectCommands.Num(), IndirectBuffer);
                        Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE);
                        Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE);
                    }
                
                }
            });
        }
    };
}