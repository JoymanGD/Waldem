#pragma once
#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
#include "Waldem/ECS/Components/BloomPostProcess.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Light.h"
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
    
    class WALDEM_API PostProcessSystem : public DrawSystem
    {
        RenderTarget* TargetRT = nullptr;
        //Post process pass
        Pipeline* PostProcessPipeline = nullptr;
        ComputeShader* PostProcessComputeShader = nullptr;
        RenderTarget* TargetRTBack = nullptr;
        Point3 GroupCount;
        PostProcessRootConstants RootConstants;
        Buffer* BloomParamsBuffer = nullptr;
        
    public:
        PostProcessSystem(ECSManager* eCSManager) : DrawSystem(eCSManager) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            for (auto [entity, bloom] : Manager->EntitiesWith<BloomPostProcess>())
            {
                TargetRT = resourceManager->GetRenderTarget("TargetRT");
                
                Vector2 resolution = Vector2(TargetRT->GetWidth(), TargetRT->GetHeight());
                bloom.TexelSize = Vector2(1.f / resolution.x, 1.f / resolution.y);
                
                TargetRTBack = resourceManager->CreateRenderTarget("TargetRTBack", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);

                BloomParamsBuffer = Renderer::CreateBuffer("BloomParamsBuffer", StorageBuffer, &bloom, sizeof(BloomPostProcess), sizeof(BloomPostProcess));

                RootConstants.TargetRT = TargetRT->GetIndex();
                RootConstants.TargetRTBack = TargetRTBack->GetIndex();
                RootConstants.BloomParamsBuffer = BloomParamsBuffer->GetIndex();
                
                PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
                PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessComputeShader);
                
                Point3 numThreads = Renderer::GetNumThreadsPerGroup(PostProcessComputeShader);
                GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
            }

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(PostProcessComputeShader) PostProcessComputeShader->Destroy();
            if(PostProcessPipeline) PostProcessPipeline->Destroy();
            if(TargetRTBack) Renderer::Destroy(TargetRTBack);
            IsInitialized = false;
        }

        void Update(float deltaTime) override
        {
            if(!IsInitialized)
                return;
            
            //Fill back buffer
            Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, COPY_SOURCE);
            Renderer::ResourceBarrier(TargetRTBack, ALL_SHADER_RESOURCE, COPY_DEST);
            Renderer::CopyResource(TargetRTBack, TargetRT);
            Renderer::ResourceBarrier(TargetRT, COPY_SOURCE, UNORDERED_ACCESS);
            Renderer::ResourceBarrier(TargetRTBack, COPY_DEST, ALL_SHADER_RESOURCE);

            for (auto [entity, bloom] : Manager->EntitiesWith<BloomPostProcess>())
            {
                Renderer::UploadBuffer(BloomParamsBuffer, &bloom, sizeof(BloomPostProcess));
            }
            
            //Post process pass
            Renderer::SetPipeline(PostProcessPipeline);
            Renderer::PushConstants(&RootConstants, sizeof(PostProcessRootConstants));
            Renderer::Compute(GroupCount);
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }

        void OnResize(Vector2 size) override
        {
            
        }
    };
}