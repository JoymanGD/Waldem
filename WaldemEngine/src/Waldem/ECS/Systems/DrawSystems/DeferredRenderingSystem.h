#pragma once
#include "Platform/OS/Windows/WindowsWindow.h"
#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
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
    struct TexturesData
    {
        uint AlbedoRT;
        uint MeshIDRT;
        uint RadianceRT;
        uint TargetRT;
        uint DummyTexture;
    };
    
    struct BuffersData
    {
        uint HoveredMeshes;
    };
    
    struct DeferredRootConstants
    {
        uint TexturesIndicesBuffer;
        uint BuffersIndicesBuffer;
        Point2 MousePos;
    };
    
    class WALDEM_API DeferredRenderingSystem : public DrawSystem
    {
        RenderTarget* TargetRT = nullptr;
        Texture2D* DummyTexture = nullptr;
        RenderTarget* AlbedoRT = nullptr;
        RenderTarget* RadianceRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        //Deferred rendering pass
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        Point3 GroupCount;
        TexturesData TexturesData;
        BuffersData BuffersData;
        DeferredRootConstants RootConstants;
        Buffer* HoveredMeshesBuffer = nullptr;
        
    public:
        DeferredRenderingSystem(ECSManager* eCSManager) : DrawSystem(eCSManager)
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            DummyTexture = Renderer::CreateTexture("DummyTexture", 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data); 
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            if(Manager->EntitiesWith<MeshComponent, Transform>().Count() <= 0)
                return;
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            
            AlbedoRT = resourceManager->GetRenderTarget("ColorRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            
            Vector2 resolution = Vector2(TargetRT->GetWidth(), TargetRT->GetHeight());

            //Deferred rendering pass
            TexturesData.AlbedoRT = resourceManager->GetRenderTarget("ColorRT")->GetIndex(SRV_UAV_CBV);
            TexturesData.MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT")->GetIndex(SRV_UAV_CBV);
            TexturesData.RadianceRT = resourceManager->GetRenderTarget("RadianceRT")->GetIndex(SRV_UAV_CBV);
            TexturesData.TargetRT = resourceManager->GetRenderTarget("TargetRT")->GetIndex(SRV_UAV_CBV);

            HoveredMeshesBuffer = Renderer::CreateBuffer("HoveredMeshes", StorageBuffer, nullptr, sizeof(int), sizeof(int));
            
            BuffersData.HoveredMeshes = HoveredMeshesBuffer->GetIndex(SRV_UAV_CBV);

            RootConstants.TexturesIndicesBuffer = Renderer::CreateBuffer("TexturesIndicesBuffer", StorageBuffer, &TexturesData, sizeof(TexturesData), sizeof(TexturesData))->GetIndex(SRV_UAV_CBV);
            RootConstants.BuffersIndicesBuffer = Renderer::CreateBuffer("BuffersIndicesBuffer", StorageBuffer, &BuffersData, sizeof(BuffersData), sizeof(BuffersData))->GetIndex(SRV_UAV_CBV);
            
            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);

            IsInitialized = true;
        }

        void Deinitialize() override
        {
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
            auto mousePos = Input::GetMousePos();
            Point2 relativeMousePos = Renderer::GetEditorViewport()->TransformMousePosition(mousePos);
            RootConstants.MousePos = relativeMousePos;
            Renderer::PushConstants(&RootConstants, sizeof(DeferredRootConstants));
            Renderer::Compute(GroupCount);
            int hoveredEntityId = 0;
            Renderer::ReadbackBuffer(HoveredMeshesBuffer, &hoveredEntityId);
            Editor::HoveredEntityID = hoveredEntityId - 1;
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }

        void OnResize(Vector2 size) override
        {
        }
    };
}
