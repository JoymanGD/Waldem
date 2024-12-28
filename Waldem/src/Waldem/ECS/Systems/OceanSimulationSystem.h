#pragma once
#include "System.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API OceanSimulationSystem : ISystem
    {
        Pipeline* OceanSimulationPipeline = nullptr;
        RootSignature* OceanSimulationRootSignature = nullptr;
        ComputeShader* OceanSimulationComputeShader = nullptr;
        RenderTarget* SpectrumRenderTarget = nullptr;
        RenderTarget* HeightmapRenderTarget = nullptr;
        Point3 GroupCount;
        
    public:
        OceanSimulationSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            SpectrumRenderTarget = resourceManager->CreateRenderTarget("SpectrumRenderTarget", 1024, 1024, TextureFormat::R32G32B32A32_FLOAT);
            Renderer::ResourceBarrier(SpectrumRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            HeightmapRenderTarget = resourceManager->CreateRenderTarget("HeightmapRenderTarget", 1024, 1024, TextureFormat::R32G32B32A32_FLOAT);
            Renderer::ResourceBarrier(HeightmapRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);

            auto resolution = Vector2(HeightmapRenderTarget->GetWidth(), HeightmapRenderTarget->GetHeight());
            WArray<Resource> resources;
            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, &resolution, sizeof(Vector2), sizeof(Vector2), 0));
            resources.Add(Resource("SpectrumRenderTarget", SpectrumRenderTarget, 0, true));
            resources.Add(Resource("HeightmapRenderTarget", HeightmapRenderTarget, 1, true));
            OceanSimulationRootSignature = Renderer::CreateRootSignature(resources);
            OceanSimulationComputeShader = Renderer::LoadComputeShader("OceanSimulation");
            OceanSimulationPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", OceanSimulationRootSignature, OceanSimulationComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(OceanSimulationComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
    
            //Do all stuff regarding pipeline, root signature, resources, etc.
        }
    
        void Update(float deltaTime) override
        {
            Renderer::SetPipeline(OceanSimulationPipeline);
            Renderer::SetRootSignature(OceanSimulationRootSignature);
            Renderer::Compute(GroupCount);
        }
    };
}
