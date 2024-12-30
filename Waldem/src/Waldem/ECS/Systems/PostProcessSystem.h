#pragma once
#include "System.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct BloomParams
    {
        float BrightThreshold;
        float BloomIntensity;
        Vector2 TexelSize;
    };

    class WALDEM_API PostProcessSystem : ISystem
    {
        RenderTarget* TargetRT = nullptr;
        //Post process pass
        Pipeline* PostProcessPipeline = nullptr;
        ComputeShader* PostProcessComputeShader = nullptr;
        RootSignature* PostProcessRootSignature = nullptr;
        RenderTarget* TargetRTBack = nullptr;
        Point3 GroupCount;
        
    public:
        PostProcessSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            
            TargetRTBack = resourceManager->CreateRenderTarget("TargetRTBack", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
            WArray<Resource> postProcessPassResources;
            postProcessPassResources.Add(Resource("TargetRT", TargetRT, 0, true));
            postProcessPassResources.Add(Resource("TargetRTBack", TargetRTBack, 0));
            postProcessPassResources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, &resolution, sizeof(Vector2), sizeof(Vector2), 0));
            BloomParams bloomParams = {};
            bloomParams.BrightThreshold = 0.5f;
            bloomParams.BloomIntensity = .6f;
            bloomParams.TexelSize = Vector2(1.f / resolution.x, 1.f / resolution.y);
            postProcessPassResources.Add(Resource("BloomParams", RTYPE_ConstantBuffer, &bloomParams, sizeof(BloomParams), sizeof(BloomParams), 1));
            PostProcessRootSignature = Renderer::CreateRootSignature(postProcessPassResources);
            PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
            PostProcessPipeline = Renderer::CreateComputePipeline("PostProcessPipeline", PostProcessRootSignature, PostProcessComputeShader);
            
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(PostProcessComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
        }

        void Update(float deltaTime) override
        {
            //Fill back buffer
            Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, COPY_SOURCE);
            Renderer::ResourceBarrier(TargetRTBack, ALL_SHADER_RESOURCE, COPY_DEST);
            Renderer::CopyRenderTarget(TargetRTBack, TargetRT);
            Renderer::ResourceBarrier(TargetRT, COPY_SOURCE, UNORDERED_ACCESS);
            Renderer::ResourceBarrier(TargetRTBack, COPY_DEST, ALL_SHADER_RESOURCE);
            
            //Post process pass
            Renderer::SetPipeline(PostProcessPipeline);
            Renderer::SetRootSignature(PostProcessRootSignature);
            Renderer::Compute(GroupCount);
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}