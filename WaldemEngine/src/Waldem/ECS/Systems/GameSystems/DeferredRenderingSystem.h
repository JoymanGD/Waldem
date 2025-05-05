#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"

namespace Waldem
{
    struct DefferedContantData
    {
        Matrix4 view;
        Matrix4 proj;
        Matrix4 invView;
        Matrix4 invProj;
        int NumLights;
    };
    
    class WALDEM_API DeferredRenderingSystem : public ISystem
    {
        RenderTarget* TargetRT = nullptr;
        Texture2D* DummyTexture = nullptr;
        RenderTarget* AlbedoRT = nullptr;
        RenderTarget* RadianceRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        //Deferred rendering pass
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        RootSignature* DeferredRenderingRootSignature = nullptr;
        Point3 GroupCount;
        DefferedContantData ConstantData;
        
    public:
        DeferredRenderingSystem(ECSManager* eCSManager) : ISystem(eCSManager)
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            int width = 1;
            int height = 1;

            TextureFormat format = TextureFormat::R8G8B8A8_UNORM;

            DummyTexture = Renderer::CreateTexture("DummyTexture", width, height, format, sizeof(Vector4), image_data); 
        }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            if(Manager->EntitiesWith<MeshComponent, Transform>().Count() <= 0)
                return;
            
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            
            AlbedoRT = resourceManager->GetRenderTarget("ColorRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");

            //Deferred rendering pass
            WArray<GraphicResource> deferredRenderingPassResources;
            deferredRenderingPassResources.Add(GraphicResource("AlbedoRT", AlbedoRT, 0));
            deferredRenderingPassResources.Add(GraphicResource("MeshIDRT", MeshIDRT, 1));
            deferredRenderingPassResources.Add(GraphicResource("RadianceRT", RadianceRT, 2));
            deferredRenderingPassResources.Add(GraphicResource("TargetRT", TargetRT, 0, true));
            deferredRenderingPassResources.Add(GraphicResource("HoveredMeshes", RTYPE_RWBuffer, nullptr, sizeof(int), sizeof(int), 1));
            deferredRenderingPassResources.Add(GraphicResource("RootConstants", RTYPE_Constant, nullptr, sizeof(float) * 2, sizeof(float) * 2, 0));
            DeferredRenderingRootSignature = Renderer::CreateRootSignature(deferredRenderingPassResources);
            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingRootSignature, DeferredRenderingComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(DeferredRenderingRootSignature) DeferredRenderingRootSignature->Destroy();
            if(DeferredRenderingComputeShader) DeferredRenderingComputeShader->Destroy();
            if(DeferredRenderingPipeline) DeferredRenderingPipeline->Destroy();
            IsInitialized = false;
        }

        void Update(float deltaTime) override
        {
            if(!IsInitialized)
                return;
            
            Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::SetPipeline(DeferredRenderingPipeline);
            Renderer::SetRootSignature(DeferredRenderingRootSignature);
            
            auto mousePos = Input::GetMousePos();
            DeferredRenderingRootSignature->UpdateResourceData("RootConstants", &mousePos);
            Renderer::Compute(GroupCount);
            int hoveredEntityId = 0;
            DeferredRenderingRootSignature->ReadbackResourceData("HoveredMeshes", &hoveredEntityId);
            Editor::HoveredEntityID = hoveredEntityId - 1;
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}
