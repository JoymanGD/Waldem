#pragma once
#include "Waldem/Input/Input.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"


namespace Waldem
{
    struct EntitySelectionRootConstants
    {
        Point2 MousePos;
        uint MeshIDRT;
        uint HoveredMeshes;
    };
    
    class WALDEM_API EntitySelectionSystem : public ISystem
    {
        //Deferred
        Pipeline* EntitySelectionPipeline = nullptr;
        ComputeShader* EntitySelectionComputeShader = nullptr;
        Point3 GroupCount;
        EntitySelectionRootConstants RootConstants;
        Buffer* HoveredMeshesBuffer = nullptr;
        
    public:
        EntitySelectionSystem()
        {
        }
        
        void Initialize(InputManager* inputManager) override
        {
            HoveredMeshesBuffer = Renderer::CreateBuffer("HoveredMeshes", StorageBuffer, sizeof(Point2), sizeof(Point2));
            RootConstants.HoveredMeshes = HoveredMeshesBuffer->GetIndex(UAV);

            EntitySelectionComputeShader = Renderer::LoadComputeShader("EntitySelection");
            EntitySelectionPipeline = Renderer::CreateComputePipeline("EntitySelectionPipeline", EntitySelectionComputeShader);

            ECS::World.system("EntitySelectionSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();

                if(viewport != ViewportManager::GetEditorViewport())
                {
                    return;
                }
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto gbuffer = viewport->GetGBuffer();
                    
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    
                    RootConstants.MeshIDRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                    Renderer::SetPipeline(EntitySelectionPipeline);
                    auto mousePos = Input::GetMousePos();
                    Point2 relativeMousePos = viewport->TransformMousePosition(mousePos);
                    RootConstants.MousePos = relativeMousePos;
                    Renderer::PushConstants(&RootConstants, sizeof(RootConstants));
                    
                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(EntitySelectionComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    Renderer::Compute(GroupCount);
                    Point2 hoveredEntityId = {};
                    Renderer::DownloadBuffer(HoveredMeshesBuffer, &hoveredEntityId, sizeof(Point2));
                    Editor::HoveredEntityType = hoveredEntityId.r;
                    Editor::HoveredEntityID = hoveredEntityId.g - 1;
                }
            });
        }
    };
}
