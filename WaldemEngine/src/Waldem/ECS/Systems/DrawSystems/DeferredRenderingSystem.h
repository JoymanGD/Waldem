#pragma once
#include <FlecsUtils.h>

#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"

namespace Waldem
{
    struct DeferredRootConstants
    {
        Point2 MousePos;
        uint AlbedoRT;
        uint MeshIDRT;
        uint RadianceRT;
        uint TargetRT;
        uint HoveredMeshes;
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
        DeferredRootConstants RootConstants;
        Buffer* HoveredMeshesBuffer = nullptr;
        
    public:
        DeferredRenderingSystem() : DrawSystem()
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            DummyTexture = Renderer::CreateTexture("DummyTexture", 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data); 
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            AlbedoRT = resourceManager->GetRenderTarget("ColorRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            
            //Deferred rendering pass
            HoveredMeshesBuffer = Renderer::CreateBuffer("HoveredMeshes", StorageBuffer, sizeof(int), sizeof(int));
            RootConstants.HoveredMeshes = HoveredMeshesBuffer->GetIndex(SRV_UAV_CBV);

            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingComputeShader);

            ECS::World.system("DeferredRenderingSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ClearRenderTarget(TargetRT);
                Renderer::ResourceBarrier(TargetRT, RENDER_TARGET, UNORDERED_ACCESS);
                Renderer::SetPipeline(DeferredRenderingPipeline);
                auto mousePos = Input::GetMousePos();
                Point2 relativeMousePos = Renderer::GetEditorViewport()->TransformMousePosition(mousePos);
                RootConstants.MousePos = relativeMousePos;
                Renderer::PushConstants(&RootConstants, sizeof(DeferredRootConstants));
                
                Vector2 resolution = Vector2(TargetRT->GetWidth(), TargetRT->GetHeight());
                Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
                GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                
                Renderer::Compute(GroupCount);
                int hoveredEntityId = 0;
                Renderer::DownloadBuffer(HoveredMeshesBuffer, &hoveredEntityId, sizeof(int));
                Editor::HoveredEntityID = hoveredEntityId - 1;
                Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            });
        }

        void OnResize(Vector2 size) override
        {
            RootConstants.AlbedoRT = AlbedoRT->GetIndex(SRV_UAV_CBV);
            RootConstants.MeshIDRT = MeshIDRT->GetIndex(SRV_UAV_CBV);
            RootConstants.RadianceRT = RadianceRT->GetIndex(SRV_UAV_CBV);
            RootConstants.TargetRT = TargetRT->GetIndex(SRV_UAV_CBV);
        }
    };
}
