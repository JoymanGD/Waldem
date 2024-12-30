#pragma once
#include "System.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Application.h"
#include "Waldem/Time.h"

namespace Waldem
{
    class WALDEM_API OceanSimulationSystem : ISystem
    {
        Pipeline* GaussianNoisePipeline = nullptr;
        RootSignature* GaussianNoiseRootSignature = nullptr;
        ComputeShader* GaussianNoiseComputeShader = nullptr;
        bool GenerateGaussianNoise = true;
        
        Pipeline* OceanSimulationPipeline = nullptr;
        RootSignature* OceanSimulationRootSignature = nullptr;
        ComputeShader* OceanSimulationComputeShader = nullptr;
        RenderTarget* SpectrumRenderTarget = nullptr;
        RenderTarget* HeightmapRenderTarget = nullptr;
        RenderTarget* GaussianNoiseRenderTarget = nullptr;
        Point3 GroupCount;
        
    public:
        OceanSimulationSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            inputManager->SubscribeToKeyEvent(G, [&](bool isPressed) 
            {
                if(isPressed)
                {
                    GenerateGaussianNoise = true;
                }
            });
            
            GaussianNoiseRenderTarget = resourceManager->GetRenderTarget("DebugRT_1");
            SpectrumRenderTarget = resourceManager->GetRenderTarget("DebugRT_2");
            HeightmapRenderTarget = resourceManager->GetRenderTarget("DebugRT_3");
    
            WArray<Resource> resources;
            resources.Add(Resource("GaussianNoiseRenderTarget", GaussianNoiseRenderTarget, 0, true));
            resources.Add(Resource("MyPushConstants", RTYPE_Constant, nullptr, sizeof(float), sizeof(float), 0));
            GaussianNoiseRootSignature = Renderer::CreateRootSignature(resources);
            GaussianNoiseComputeShader = Renderer::LoadComputeShader("GaussianNoise");
            GaussianNoisePipeline = Renderer::CreateComputePipeline("GaussianNoisePipeline", GaussianNoiseRootSignature, GaussianNoiseComputeShader);
            resources.Clear();
            
            auto resolution = Vector2(HeightmapRenderTarget->GetWidth(), HeightmapRenderTarget->GetHeight());
            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, &resolution, sizeof(Vector2), sizeof(Vector2), 0));
            resources.Add(Resource("SpectrumRenderTarget", SpectrumRenderTarget, 0, true));
            resources.Add(Resource("HeightmapRenderTarget", HeightmapRenderTarget, 1, true));
            resources.Add(Resource("GaussianNoiseRenderTarget", GaussianNoiseRenderTarget, 0));
            OceanSimulationRootSignature = Renderer::CreateRootSignature(resources);
            OceanSimulationComputeShader = Renderer::LoadComputeShader("OceanSimulation");
            OceanSimulationPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", OceanSimulationRootSignature, OceanSimulationComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(OceanSimulationComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
        }
    
        void Update(float deltaTime) override
        {
            if(GenerateGaussianNoise)
            {
                Renderer::ResourceBarrier(GaussianNoiseRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                Renderer::SetPipeline(GaussianNoisePipeline);
                Renderer::SetRootSignature(GaussianNoiseRootSignature);
                GaussianNoiseRootSignature->UpdateResourceData("MyPushConstants", &Time::ElapsedTime);
                Renderer::Compute(GroupCount);
                GenerateGaussianNoise = false;
                Renderer::ResourceBarrier(GaussianNoiseRenderTarget, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            }
            
            Renderer::ResourceBarrier(SpectrumRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::ResourceBarrier(HeightmapRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::SetPipeline(OceanSimulationPipeline);
            Renderer::SetRootSignature(OceanSimulationRootSignature);
            Renderer::Compute(GroupCount);
            Renderer::ResourceBarrier(SpectrumRenderTarget, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            Renderer::ResourceBarrier(HeightmapRenderTarget, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}
