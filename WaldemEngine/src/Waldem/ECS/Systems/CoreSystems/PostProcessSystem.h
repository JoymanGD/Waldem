#pragma once
#include <FlecsUtils.h>

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/BloomPostProcess.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/GBuffer.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct PostProcessRootConstants
    {
        uint TargetRT;
        uint PostProcess;
        uint BloomParamsBuffer;
    };
    
    class WALDEM_API PostProcessSystem : public ICoreSystem
    {
        //Post process pass
        Pipeline* PostProcessPipeline = nullptr;
        ComputeShader* PostProcessComputeShader = nullptr;
        PostProcessRootConstants RootConstants;
        Buffer* BloomParamsBuffer = nullptr;
        
    public:
        PostProcessSystem() {}
        
        void Initialize() override
        {
            ECS::World.observer<BloomPostProcess>().event(flecs::OnAdd).each([&](flecs::entity entity, BloomPostProcess& bloom)
            {
                if(!IsInitialized)
                {
                    BloomParamsBuffer = Renderer::CreateBuffer("BloomParamsBuffer", StorageBuffer, sizeof(BloomPostProcess), sizeof(BloomPostProcess));

                    RootConstants.BloomParamsBuffer = BloomParamsBuffer->GetIndex(SRV_CBV);
                    
                    PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
                    PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessComputeShader);
                    
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
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    auto postProcessRT = viewport->GetGBufferRenderTarget(PostProcess);
                    
                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(PostProcessComputeShader);
                    auto groupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    Renderer::ResourceBarrier(deferredRT, ALL_SHADER_RESOURCE, COPY_SOURCE);
                    Renderer::ResourceBarrier(postProcessRT, ALL_SHADER_RESOURCE, COPY_DEST);
                    Renderer::CopyResource(postProcessRT, deferredRT);
                    Renderer::ResourceBarrier(deferredRT, COPY_SOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(postProcessRT, COPY_DEST, ALL_SHADER_RESOURCE);

                    Renderer::UploadBuffer(BloomParamsBuffer, &bloom, sizeof(BloomPostProcess));
                    
                    //Post process pass
                    RootConstants.PostProcess = postProcessRT->GetIndex(SRV_CBV);
                    Renderer::SetPipeline(PostProcessPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    Renderer::ResourceBarrier(deferredRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}