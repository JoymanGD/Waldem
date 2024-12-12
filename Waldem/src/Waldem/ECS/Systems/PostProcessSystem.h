#pragma once
#include "System.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API PostProcessSystem : ISystem
    {
        ComputeShader* PostProcessComputeShader = nullptr;
        RenderTarget* TestRenderTarget = nullptr;
        
    public:
        PostProcessSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override
        {
            WArray<Resource> resources;
            TestRenderTarget = Renderer::CreateRenderTarget("TestRenderTarget", 1024, 1024, TextureFormat::TEXTURE_FORMAT_R32_FLOAT);
            resources.Add(Resource("TestResource", TestRenderTarget, 0));
            PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess", resources);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            Point3 numThreads = sceneData->Renderer->GetNumThreadsPerGroup(PostProcessComputeShader);
            Renderer::Compute(PostProcessComputeShader, Point3((TestRenderTarget->GetWidth() + numThreads.x - 1) / numThreads.x, (TestRenderTarget->GetWidth() + numThreads.y - 1) / numThreads.y, 1));
        }
    };
}
