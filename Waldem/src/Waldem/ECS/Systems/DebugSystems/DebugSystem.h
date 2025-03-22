#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    struct DebugSystemConstantBuffer
    {
        Vector2 TargetResolution;
        Vector2 DebugResolution;
        uint32_t DebugRTIndex;
    };
    
    class WALDEM_API DebugSystem : ISystem
    {
        Matrix4 CachedViewProjMatrix;
        Vector3 frustumCorners[8];

        Vector2 MousePos = { 0, 0 };

        //Displaying debugRTs
        bool DisplayDebugRTs = false;
        Pipeline* DebugRenderTargetsPipeline = nullptr;
        RootSignature* DebugRenderTargetsRootSignature = nullptr;
        ComputeShader* DebugRenderTargetsComputeShader = nullptr;
        RenderTarget* TargetRT = nullptr;
        RenderTarget* DebugRT_1 = nullptr;
        RenderTarget* DebugRT_2 = nullptr;
        RenderTarget* DebugRT_3 = nullptr;
        RenderTarget* DebugRT_4 = nullptr;
        RenderTarget* DebugRT_5 = nullptr;
        RenderTarget* DebugRT_6 = nullptr;
        RenderTarget* DebugRT_7 = nullptr;
        RenderTarget* DebugRT_8 = nullptr;
        RenderTarget* DebugRT_9 = nullptr;
        Point3 GroupCount;
        DebugSystemConstantBuffer ConstantBufferData;
        
    public:
        DebugSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}

        void CacheFrustrumCorners()
        {
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                Vector3 ndcCorners[8] =
                {
                    {-1.0f, -1.0f, 0}, //near-top-left
                    {1.0f, -1.0f, 0}, //near-top-right
                    {-1.0f, 1.0f, 0}, //near-bottom-left
                    {1.0f, 1.0f, 0}, //near-bottom-right
                    {-1.0f, -1.0f, 1.0f}, //far-top-left
                    {1.0f, -1.0f, 1.0f}, //far-top-right
                    {-1.0f, 1.0f, 1.0f}, //far-bottom-left
                    {1.0f, 1.0f, 1.0f}  //far-bottom-right
                };

                // Transform NDC corners to world space
                CachedViewProjMatrix = camera.ProjectionMatrix * camera.ViewMatrix;
                auto invViewProjMatrix = inverse(CachedViewProjMatrix);
                
                for (int i = 0; i < 8; ++i)
                {
                    Vector4 corner(ndcCorners[i].x, ndcCorners[i].y, ndcCorners[i].z, 1.0f);
                    Vector4 worldPos = invViewProjMatrix * corner; // Use your matrix transform function
                    worldPos /= worldPos.w; // Perform perspective divide
                    frustumCorners[i] = Vector3(worldPos.x, worldPos.y, worldPos.z);
                }
                break;
            }
        }

        // void DrawFrustum(Vector4 color, Matrix4 viewProjMatrix)
        // {
        //     WArray<LineData> lines;
        //
        //     //near plane
        //     lines.Add({ frustumCorners[0], frustumCorners[1], color }); //top
        //     lines.Add({ frustumCorners[1], frustumCorners[3], color }); //right
        //     lines.Add({ frustumCorners[3], frustumCorners[2], color }); //bottom
        //     lines.Add({ frustumCorners[2], frustumCorners[0], color }); //left
        //
        //     //far plane
        //     lines.Add({ frustumCorners[4], frustumCorners[5], color }); //top
        //     lines.Add({ frustumCorners[5], frustumCorners[7], color }); //right
        //     lines.Add({ frustumCorners[7], frustumCorners[6], color }); //bottom
        //     lines.Add({ frustumCorners[6], frustumCorners[4], color }); //left
        //
        //     //connect near plane to far plane
        //     lines.Add({ frustumCorners[0], frustumCorners[4], color }); //top-left
        //     lines.Add({ frustumCorners[1], frustumCorners[5], color }); //top-right
        //     lines.Add({ frustumCorners[2], frustumCorners[6], color }); //bottom-left
        //     lines.Add({ frustumCorners[3], frustumCorners[7], color }); //bottom-right
        //
        //     // for (auto& line : lines)
        //     // {
        //     //     line.ToClipSpace(viewProjMatrix);
        //     // }
        // }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            CacheFrustrumCorners();

            inputManager->SubscribeToMouseMoveEvent([&](Vector2 mousePos)
            {
                MousePos = mousePos;
            });

            inputManager->SubscribeToKeyEvent(KP_0, [&](bool isPressed)
            {
                if(isPressed)
                {
                    DisplayDebugRTs = !DisplayDebugRTs;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_1, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 1;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_2, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 2;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_3, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 3;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_4, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 4;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_5, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 5;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_6, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 6;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_7, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 7;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_8, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 8;
                }
            });

            inputManager->SubscribeToKeyEvent(KP_9, [&](bool isPressed)
            {
                if(isPressed)
                {
                    ConstantBufferData.DebugRTIndex = 9;
                }
            });

            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            DebugRT_1 = resourceManager->GetRenderTarget("DebugRT_1");
            DebugRT_2 = resourceManager->GetRenderTarget("DebugRT_2");
            DebugRT_3 = resourceManager->GetRenderTarget("DebugRT_3");
            DebugRT_4 = resourceManager->GetRenderTarget("DebugRT_4");
            DebugRT_5 = resourceManager->GetRenderTarget("DebugRT_5");
            DebugRT_6 = resourceManager->GetRenderTarget("DebugRT_6");
            DebugRT_7 = resourceManager->GetRenderTarget("DebugRT_7");
            DebugRT_8 = resourceManager->GetRenderTarget("DebugRT_8");
            DebugRT_9 = resourceManager->GetRenderTarget("DebugRT_9");
            
            ConstantBufferData.TargetResolution = Vector2(TargetRT->GetWidth(), TargetRT->GetHeight());
            ConstantBufferData.DebugResolution = Vector2(DebugRT_1->GetWidth(), DebugRT_1->GetHeight());
            ConstantBufferData.DebugRTIndex = 1;
            
            WArray<Resource> resources;
            resources.Add(Resource("TargetRT", TargetRT, 0, true));
            resources.Add(Resource("DebugRT_1", DebugRT_1, 0));
            resources.Add(Resource("DebugRT_2", DebugRT_2, 1));
            resources.Add(Resource("DebugRT_3", DebugRT_3, 2));
            resources.Add(Resource("DebugRT_4", DebugRT_4, 3));
            resources.Add(Resource("DebugRT_5", DebugRT_5, 4));
            resources.Add(Resource("DebugRT_6", DebugRT_6, 5));
            resources.Add(Resource("DebugRT_7", DebugRT_7, 6));
            resources.Add(Resource("DebugRT_8", DebugRT_8, 7));
            resources.Add(Resource("DebugRT_9", DebugRT_9, 8));
            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, &ConstantBufferData, sizeof(DebugSystemConstantBuffer), sizeof(DebugSystemConstantBuffer), 0));
            DebugRenderTargetsRootSignature = Renderer::CreateRootSignature(resources);
            DebugRenderTargetsComputeShader = Renderer::LoadComputeShader("DebugRenderTargets");
            DebugRenderTargetsPipeline = Renderer::CreateComputePipeline("DebugRenderTargetsPipeline", DebugRenderTargetsRootSignature, DebugRenderTargetsComputeShader);
            
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(DebugRenderTargetsComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
        }

        void Update(float deltaTime) override
        {
            if(DisplayDebugRTs)
            {
                DebugRenderTargetsRootSignature->UpdateResourceData("MyConstantBuffer", &ConstantBufferData);
                Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                Renderer::SetPipeline(DebugRenderTargetsPipeline);
                Renderer::SetRootSignature(DebugRenderTargetsRootSignature);
                Renderer::Compute(GroupCount);
                Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            }
        }
    };
}
