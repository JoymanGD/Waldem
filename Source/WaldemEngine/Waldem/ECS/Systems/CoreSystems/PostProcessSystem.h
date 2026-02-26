#pragma once
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
            
            ECS::World.system<PostProcessComponent>("BloomPostProcessSystem").kind<ECS::OnDraw>().each([&](ECS::Entity entity, PostProcessComponent&)
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
                    
                    Renderer::SetPipeline(PostProcessPipeline);
                    
                    Renderer::ResourceBarrier(pingRT, UNORDERED_ACCESS);
                    RootConstants.SrcRT = deferredRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pingRT->GetIndex(UAV);
                    RootConstants.Stage = 1; // Bright pass
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    
                    Renderer::ResourceBarrier(pingRT, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(pongRT, UNORDERED_ACCESS);
                    RootConstants.SrcRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pongRT->GetIndex(UAV);
                    RootConstants.Stage = 2; // Horizontal
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    
                    Renderer::ResourceBarrier(pongRT, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(pingRT, UNORDERED_ACCESS);
                    RootConstants.SrcRT = pongRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = pingRT->GetIndex(UAV);
                    RootConstants.Stage = 3; // Vertical
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    
                    Renderer::ResourceBarrier(pingRT, ALL_SHADER_RESOURCE);
                    Renderer::ResourceBarrier(deferredRT, UNORDERED_ACCESS);
                    RootConstants.SrcRT = pingRT->GetIndex(SRV_CBV);
                    RootConstants.DstRT = deferredRT->GetIndex(UAV);
                    RootConstants.Stage = 4; // Composite
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    
                    Renderer::UAVBarrier(deferredRT);
                    RootConstants.DstRT = deferredRT->GetIndex(UAV);
                    RootConstants.Stage = 5; // Tonemapping
                    Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
                    Renderer::Compute(groupCount);
                    
                    Renderer::ResourceBarrier(deferredRT, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}