#pragma once
#include <FlecsUtils.h>

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/BloomPostProcess.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct PostProcessRootConstants
    {
        uint TargetRT;
        uint TargetRTBack;
        uint BloomParamsBuffer;
    };
    
    class WALDEM_API PostProcessSystem : public ISystem
    {
        RenderTarget* TargetRT = nullptr;
        //Post process pass
        Pipeline* PostProcessPipeline = nullptr;
        ComputeShader* PostProcessComputeShader = nullptr;
        RenderTarget* TargetRTBack = nullptr;
        Point3 GroupCount;
        PostProcessRootConstants RootConstants;
        Buffer* BloomParamsBuffer = nullptr;
        Vector2 TexelSize;
        
    public:
        PostProcessSystem() {}
        
        void Initialize(InputManager* inputManager) override
        {
            ECS::World.observer<BloomPostProcess>().event(flecs::OnAdd).each([&](flecs::entity entity, BloomPostProcess& bloom)
            {
                if(!IsInitialized)
                {
                    TargetRT = Renderer::GetRenderTarget("TargetRT");
                    
                    Vector2 resolution = Vector2(TargetRT->GetWidth(), TargetRT->GetHeight());
                    TexelSize = Vector2(1.f / resolution.x, 1.f / resolution.y);
                    
                    TargetRTBack = Renderer::CreateRenderTarget("TargetRTBack", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);

                    BloomParamsBuffer = Renderer::CreateBuffer("BloomParamsBuffer", StorageBuffer, sizeof(BloomPostProcess), sizeof(BloomPostProcess));

                    RootConstants.TargetRT = TargetRT->GetIndex(SRV_CBV);
                    RootConstants.TargetRTBack = TargetRTBack->GetIndex(SRV_CBV);
                    RootConstants.BloomParamsBuffer = BloomParamsBuffer->GetIndex(SRV_CBV);
                    
                    PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
                    PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessComputeShader);
                    
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(PostProcessComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    IsInitialized = true;
                }
                else
                {
                    WD_CORE_WARN("Entity with the bloom component already exists in the world. Deleting last added component.");
                    entity.remove<BloomPostProcess>();
                }
            });
            
            ECS::World.system<BloomPostProcess>("PostProcessBloomSystem").kind(flecs::OnDraw).each([&](BloomPostProcess& bloom)
            {
                if(IsInitialized)
                {
                    //Fill back buffer
                    Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, COPY_SOURCE);
                    Renderer::ResourceBarrier(TargetRTBack, ALL_SHADER_RESOURCE, COPY_DEST);
                    Renderer::CopyResource(TargetRTBack, TargetRT);
                    Renderer::ResourceBarrier(TargetRT, COPY_SOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(TargetRTBack, COPY_DEST, ALL_SHADER_RESOURCE);

                    Renderer::UploadBuffer(BloomParamsBuffer, &bloom, sizeof(BloomPostProcess));
                    
                    //Post process pass
                    Renderer::SetPipeline(PostProcessPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(GroupCount);
                    Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}