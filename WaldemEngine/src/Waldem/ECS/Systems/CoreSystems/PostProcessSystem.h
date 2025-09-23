#pragma once
#include <FlecsUtils.h>

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/PostProcessComponent.h"
#include "Waldem/Renderer/GBuffer.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct PostProcessRootConstants
    {
        uint SrcRT;
        uint DstRT;
        uint PostProcessParamsBuffer;
        uint Stage;
    };
    
    class WALDEM_API PostProcessSystem : public ICoreSystem
    {
        //Post process pass
        Pipeline* PostProcessPipeline = nullptr;
        ComputeShader* PostProcessComputeShader = nullptr;
        PostProcessRootConstants RootConstants;
        Buffer* PostProcessParamsBuffer = nullptr;
        
    public:
        PostProcessSystem() {}
        
        void Initialize() override
        {
            PostProcessParamsBuffer = Renderer::CreateBuffer("PostProcessParamsBuffer", StorageBuffer, sizeof(PostProcessComponent), sizeof(PostProcessComponent));

            RootConstants.PostProcessParamsBuffer = PostProcessParamsBuffer->GetIndex(SRV_CBV);
            
            PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
            PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessComputeShader);
            
            ECS::World.observer<PostProcessComponent>().event(flecs::OnAdd).each([&](PostProcessComponent& bloom)
            {
                Renderer::UploadBuffer(PostProcessParamsBuffer, &bloom, sizeof(PostProcessComponent));
            });
            
            ECS::World.observer<PostProcessComponent>().event(flecs::OnSet).each([&](PostProcessComponent& bloom)
            {
                Renderer::UploadBuffer(PostProcessParamsBuffer, &bloom, sizeof(PostProcessComponent));
            });
            
            ECS::World.system<PostProcessComponent>("BloomPostProcessSystem").kind(flecs::OnDraw).each([&](ECS::Entity entity, PostProcessComponent&)
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    viewport->GetGBuffer()->Clear({ PostProcess, Ping, Pong });
                    
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    auto postProcessRT = viewport->GetGBufferRenderTarget(PostProcess);
                    auto pingRT = viewport->GetGBufferRenderTarget(Ping);
                    auto pongRT = viewport->GetGBufferRenderTarget(Pong);
                    
                    Vector2 resolution = Vector2(postProcessRT->GetWidth(), postProcessRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(PostProcessComputeShader);
                    auto groupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    Renderer::ResourceBarrier(pingRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(pongRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(deferredRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::ResourceBarrier(postProcessRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::SetPipeline(PostProcessPipeline);
                    RootConstants.SrcRT = deferredRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = postProcessRT->GetIndex(SRV_CBV);
                    RootConstants.Stage = 0; // Copy
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = postProcessRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.Stage = 1; // Bright pass
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pongRT->GetIndex(SRV_CBV);
                    RootConstants.Stage = 2; // Horizontal
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = pongRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.Stage = 3; // Vertical
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = postProcessRT->GetIndex(SRV_CBV);
                    RootConstants.Stage = 4; // Composite
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    RootConstants.SrcRT = postProcessRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = deferredRT->GetIndex(SRV_CBV);
                    RootConstants.Stage = 5; // Tonemapping
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    Renderer::ResourceBarrier(pingRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(pongRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(postProcessRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(deferredRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}