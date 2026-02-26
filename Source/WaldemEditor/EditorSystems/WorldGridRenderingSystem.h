#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem
{
    struct SSceneData
    {
        Matrix4 InverseProjection;
        Matrix4 InverseView;
        Vector4 CameraPosition;
    };

    struct SRootConstants
    {
        uint SceneDataBuffer;
        uint DepthRTID;
        uint TargetRTID;
    };
    
    class WorldGridRenderingSystem : public ISystem
    {
        Pipeline* WorldGridRenderingPipeline = nullptr;
        ComputeShader* WorldGridRenderingComputeShader = nullptr;
        Point3 GroupCount;
        SRootConstants RootConstants;
        SSceneData SceneData;
        Buffer* SceneDataBuffer = nullptr;
        
    public:
        
        void Initialize(InputManager* inputManager) override
        {
            SceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(SSceneData), sizeof(SSceneData));
            RootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);

            WorldGridRenderingComputeShader = Renderer::LoadComputeShader("WorldGrid");
            WorldGridRenderingPipeline = Renderer::CreateComputePipeline("WorldGridPipeline", WorldGridRenderingComputeShader);
            
            ECS::World.system("WorldGridRenderingSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();

                if(viewport != ViewportManager::GetEditorViewport())
                    return;
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto& camera = linkedCamera.get<Camera>();
                    auto& transform = linkedCamera.get<Transform>();

                    SceneData.CameraPosition = Vector4(transform.Position, 1.0f);
                    SceneData.InverseView = transform.Matrix;
                    SceneData.InverseProjection = inverse(camera.ProjectionMatrix);
                    Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(SSceneData));
                    
                    auto gbuffer = viewport->GetGBuffer();
                    
                    auto targetRT = viewport->FrameBuffer->GetCurrentRenderTarget();
                    
                    RootConstants.TargetRTID = targetRT->GetIndex(UAV);
                    RootConstants.DepthRTID = gbuffer->GetRenderTarget(Depth)->GetIndex(SRV_CBV);
                    Renderer::SetPipeline(WorldGridRenderingPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(SRootConstants));
                    
                    Renderer::ResourceBarrier(targetRT, RENDER_TARGET, UNORDERED_ACCESS);
                    
                    Vector2 resolution = Vector2(targetRT->GetWidth(), targetRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(WorldGridRenderingComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    Renderer::Compute(GroupCount);
                    
                    Renderer::ResourceBarrier(targetRT, UNORDERED_ACCESS, RENDER_TARGET);
                }
            });
        }
    };
}