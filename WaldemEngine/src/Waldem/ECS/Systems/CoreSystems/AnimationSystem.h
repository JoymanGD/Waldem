#pragma once
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Components/AnimationListener.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/ResizableBuffer.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct AnimationRootConstants
    {
        uint OriginalVertexBuffer;
        uint VertexBuffer;
        uint VertexOffset;
        Matrix4 WorldTransform;
    };
    
    class WALDEM_API AnimationSystem : public ICoreSystem
    {
        //Post process pass
        Pipeline* AnimationPipeline = nullptr;
        ComputeShader* AnimationComputeShader = nullptr;
        Point3 NumThreads;
        AnimationRootConstants RootConstants;
        
    public:
        AnimationSystem() {}
        
        void Initialize() override
        {
            AnimationComputeShader = Renderer::LoadComputeShader("Animation");
            AnimationPipeline = Renderer::CreateComputePipeline("AnimationPipeline", AnimationComputeShader);
            NumThreads = Renderer::GetNumThreadsPerGroup(AnimationComputeShader);
            
            ECS::World.system<MeshComponent, Transform, AnimationListener>().kind<ECS::OnDraw>().each([&](flecs::entity entity, MeshComponent& meshComponent, Transform& transform, AnimationListener)
            {
                if(!meshComponent.MeshRef.IsValid())
                {
                    return;
                }

                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), VERTEX_AND_CONSTANT_BUFFER, UNORDERED_ACCESS);
                RootConstants.OriginalVertexBuffer = meshComponent.MeshRef.Mesh->VertexBuffer->GetIndex(SRV_CBV);
                RootConstants.VertexBuffer = Renderer::RenderData.VertexBuffer.GetIndex(UAV);
                RootConstants.WorldTransform = transform.Matrix;
                RootConstants.VertexOffset = meshComponent.DrawCommand.BaseVertexLocation;
                Renderer::SetPipeline(AnimationPipeline);
                Renderer::PushConstants(&RootConstants, sizeof(AnimationRootConstants));
                int verticesAmount = meshComponent.MeshRef.Mesh->VertexData.Num();
                Point3 GroupCount = Point3((verticesAmount + NumThreads.x - 1) / NumThreads.x, 1, 1);
                Renderer::Compute(GroupCount);
                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), UNORDERED_ACCESS, NON_PIXEL_SHADER_RESOURCE);

                int index;
                IdManager::GetId(entity, GlobalDrawIdType, index);
                Renderer::RenderData.TLAS.UpdateGeometry(index, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, meshComponent.DrawCommand, verticesAmount);
                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), NON_PIXEL_SHADER_RESOURCE, VERTEX_AND_CONSTANT_BUFFER);
            });
        }
    };
}