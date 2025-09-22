#pragma once
#include "Waldem/ECS/IdManager.h"
#include "Waldem/Input/Input.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Sky.h"
#include "Waldem/ECS/Components/Sprite.h"
#include "Waldem/Renderer/ResizableAccelerationStructure.h"
#include "Waldem/Renderer/ResizableBuffer.h"
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
        uint DeferredRT;
    };
    
    class WALDEM_API WorldGridRenderingSystem : public ICoreSystem
    {
        Pipeline* WorldGridRenderingPipeline = nullptr;
        ComputeShader* WorldGridRenderingComputeShader = nullptr;
        Point3 GroupCount;
        SRootConstants RootConstants;
        SSceneData SceneData;
        Buffer* SceneDataBuffer = nullptr;
        
    public:
        
        void Initialize() override
        {
            SceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(SSceneData), sizeof(SSceneData));
            RootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);

            WorldGridRenderingComputeShader = Renderer::LoadComputeShader("WorldGrid");
            WorldGridRenderingPipeline = Renderer::CreateComputePipeline("WorldGridPipeline", WorldGridRenderingComputeShader);
            
            ECS::World.system("WorldGridRenderingSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();

                if(viewport != ViewportManager::GetEditorViewport())
                    return;
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto camera = linkedCamera.get<Camera>();
                    auto transform = linkedCamera.get<Transform>();

                    SceneData.CameraPosition = Vector4(transform->Position, 1.0f);
                    SceneData.InverseView = transform->Matrix;
                    SceneData.InverseProjection = inverse(camera->ProjectionMatrix);
                    Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(SSceneData));
                    
                    auto gbuffer = viewport->GetGBuffer();
                    
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    
                    RootConstants.DeferredRT = deferredRT->GetIndex(SRV_CBV);
                    RootConstants.DepthRTID = gbuffer->GetRenderTarget(Depth)->GetIndex(SRV_CBV);
                    Renderer::SetPipeline(WorldGridRenderingPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(SRootConstants));
                    
                    gbuffer->Barrier(Deferred, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    
                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(WorldGridRenderingComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    Renderer::Compute(GroupCount);
                    
                    gbuffer->Barrier(Deferred, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}