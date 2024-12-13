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
        Point3 GroupCount;
        
    public:
        PostProcessSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override
        {
            WArray<Resource> resources;
            TestRenderTarget = Renderer::CreateRenderTarget("TestRenderTarget", 1024, 1024, TextureFormat::R32_FLOAT);
            resources.Add(Resource("TestResource", TestRenderTarget, 0));
            PostProcessComputeShader = Renderer::LoadComputeShader("PostProcess");
            Point3 numThreads = sceneData->Renderer->GetNumThreadsPerGroup(PostProcessComputeShader);
            GroupCount = Point3((TestRenderTarget->GetWidth() + numThreads.x - 1) / numThreads.x, (TestRenderTarget->GetWidth() + numThreads.y - 1) / numThreads.y, 1);

            //Do all stuff regarding pipeline, root signature, resources, etc.
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            //set resources, pipeline, root signature, etc.
            
            Renderer::Compute(GroupCount);
        }
    };
}
