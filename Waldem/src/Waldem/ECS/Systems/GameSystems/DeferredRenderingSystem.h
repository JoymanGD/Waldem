#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "..\..\Components\EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

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
    
    class WALDEM_API DeferredRenderingSystem : ISystem
    {
        RenderTarget* TargetRT = nullptr;
        Texture2D* DummyTexture = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* RadianceRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        //Deferred rendering pass
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        RootSignature* DeferredRenderingRootSignature = nullptr;
        Point3 GroupCount;
        DefferedContantData ConstantData;
        
    public:
        DeferredRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager)
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            int width = 1;
            int height = 1;

            TextureFormat format = TextureFormat::R8G8B8A8_UNORM;

            DummyTexture = Renderer::CreateTexture("DummyTexture", width, height, format, image_data); 
        }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            
            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            ColorRT = resourceManager->GetRenderTarget("ColorRT");
            ORMRT = resourceManager->GetRenderTarget("ORMRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT = resourceManager->GetRenderTarget("DepthRT");

            //Deferred rendering pass
            WArray<Resource> deferredRenderingPassResources;
            WArray<LightTransformData> LightTransformDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightTransformDatas.Add({ light.Type, transform.GetForwardVector(), Vector4(transform.Position, 1.0f) });
            }
            deferredRenderingPassResources.Add(Resource("ComparisonSampler", { Sampler( COMPARISON_MIN_MAG_MIP_LINEAR, WRAP, WRAP, WRAP, LESS_EQUAL) }, 1));
            if(!LightTransformDatas.IsEmpty())
                deferredRenderingPassResources.Add(Resource("LightTransformDatas", RTYPE_Buffer, nullptr, sizeof(LightTransformData), LightTransformDatas.GetSize(), 0));
            deferredRenderingPassResources.Add(Resource("WorldPosition", WorldPositionRT, 1));
            deferredRenderingPassResources.Add(Resource("Normal", NormalRT, 2));
            deferredRenderingPassResources.Add(Resource("Color", ColorRT, 3));
            deferredRenderingPassResources.Add(Resource("ORMRT", ORMRT, 4));
            deferredRenderingPassResources.Add(Resource("MeshIDRT", MeshIDRT, 5));
            deferredRenderingPassResources.Add(Resource("DepthRT", DepthRT, 6));
            deferredRenderingPassResources.Add(Resource("RadianceRT", RadianceRT, 7));
            deferredRenderingPassResources.Add(Resource("TargetRT", TargetRT, 0, true));
            deferredRenderingPassResources.Add(Resource("HoveredMeshes", RTYPE_RWBuffer, nullptr, sizeof(int), sizeof(int), 1));
            deferredRenderingPassResources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(DefferedContantData), sizeof(DefferedContantData), 0));
            deferredRenderingPassResources.Add(Resource("RootConstants", RTYPE_Constant, nullptr, sizeof(float) * 2, sizeof(float) * 2, 1));
            DeferredRenderingRootSignature = Renderer::CreateRootSignature(deferredRenderingPassResources);
            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingRootSignature, DeferredRenderingComputeShader);
            Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
            GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
        }

        void Update(float deltaTime) override
        {
            WArray<LightTransformData> LightTransformDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightTransformDatas.Add({ light.Type, transform.GetForwardVector(), Vector4(transform.Position, 1.0f) });
            }
            if(!LightTransformDatas.IsEmpty())
                DeferredRenderingRootSignature->UpdateResourceData("LightTransformDatas", LightTransformDatas.GetData());
            
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                ConstantData.view = camera.ViewMatrix;
                ConstantData.proj = camera.ProjectionMatrix;
                ConstantData.invView = cameraTransform.Matrix;
                ConstantData.invProj = inverse(camera.ProjectionMatrix);

                break;
            }
            ConstantData.NumLights = LightTransformDatas.GetSize();
            DeferredRenderingRootSignature->UpdateResourceData("MyConstantBuffer", &ConstantData);
            
            Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::SetPipeline(DeferredRenderingPipeline);
            Renderer::SetRootSignature(DeferredRenderingRootSignature);
            
            auto mousePos = Input::GetMousePos();
            DeferredRenderingRootSignature->UpdateResourceData("RootConstants", &mousePos);
            Renderer::Compute(GroupCount);
            int hoveredEntityId = 0;
            DeferredRenderingRootSignature->ReadbackResourceData("HoveredMeshes", &hoveredEntityId);
            Editor::HoveredIntityID = hoveredEntityId - 1;
            Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}
