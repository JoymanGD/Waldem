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
        uint SrcRT;
        uint DstRT;
        uint BloomParamsBuffer;
        uint BlurDirection;
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
            BloomParamsBuffer = Renderer::CreateBuffer("BloomParamsBuffer", StorageBuffer, sizeof(BloomPostProcess), sizeof(BloomPostProcess));

            RootConstants.BloomParamsBuffer = BloomParamsBuffer->GetIndex(SRV_CBV);
            
            PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
            PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessComputeShader);
            
            ECS::World.observer<BloomPostProcess>().event(flecs::OnSet).each([&](BloomPostProcess& bloom)
            {
                Renderer::UploadBuffer(BloomParamsBuffer, &bloom, sizeof(BloomPostProcess));
            });
            
            ECS::World.system("PostProcessSystem").kind(flecs::OnDraw).each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    viewport->GetGBuffer()->Clear({ Ping, Pong });
                    
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    auto pingRT = viewport->GetGBufferRenderTarget(Ping);
                    auto pongRT = viewport->GetGBufferRenderTarget(Pong);
                    
                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(PostProcessComputeShader);
                    auto groupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    Renderer::ResourceBarrier(deferredRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(pingRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(pongRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::SetPipeline(PostProcessPipeline);
                    RootConstants.SrcRT = deferredRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.BlurDirection = 0; // Bright pass
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pongRT->GetIndex(SRV_CBV);
                    RootConstants.BlurDirection = 1; // Horizontal
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = pongRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.BlurDirection = 2; // Vertical
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = deferredRT->GetIndex(SRV_CBV);
                    RootConstants.BlurDirection = 3; // Composite
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    Renderer::ResourceBarrier(deferredRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(pingRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(pongRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}