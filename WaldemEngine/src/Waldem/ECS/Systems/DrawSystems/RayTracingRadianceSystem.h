#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/GraphicResource.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/InitializedComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/ResizableAccelerationStructure.h"
#include "Waldem/Renderer/ResizableBuffer.h"

namespace Waldem
{
    struct RayTracingSceneData
    {
        Matrix4 InvViewMatrix;
        Matrix4 InvProjectionMatrix;
        int NumLights = 0;
    };

    struct RayTracingRootConstants
    {
        uint WorldPositionRT;
        uint NormalRT;
        uint ColorRT;
        uint ORMRT;
        uint OutputColorRT;
        uint LightsBuffer;
        uint LightTransformsBuffer;
        uint SceneDataBuffer; 
        uint TLAS;
    };
    
    class WALDEM_API RayTracingRadianceSystem : public DrawSystem
    {
        RenderTarget* RadianceRT = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* AlbedoRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        WArray<AccelerationStructure*> BLAS;
        WMap<CMesh*, AccelerationStructure*> BLASToUpdate;
        RayTracingSceneData RTSceneData;
        ResizableBuffer LightsBuffer;
        ResizableBuffer LightTransformsBuffer;
        Buffer* SceneDataBuffer = nullptr;
        ResizableAccelerationStructure TLAS;
        RayTracingRootConstants RootConstants;
        WArray<LightData> LightDatas;
        WArray<Matrix4> LightTransforms;
        
    public:
        RayTracingRadianceSystem() : DrawSystem() {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            RTShader = Renderer::LoadRayTracingShader("RayTracing/Radiance");
            RTPipeline = Renderer::CreateRayTracingPipeline("RayTracingPipeline", RTShader);
                    
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            AlbedoRT = resourceManager->GetRenderTarget("ColorRT");
            ORMRT = resourceManager->GetRenderTarget("ORMRT");
                    
            LightsBuffer = ResizableBuffer("LightsBuffer", StorageBuffer, sizeof(LightData), 40);
            LightTransformsBuffer = ResizableBuffer("LightTransformsBuffer", StorageBuffer, sizeof(Matrix4), 40);
            SceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, sizeof(RayTracingSceneData), sizeof(RayTracingSceneData), &RTSceneData);
            TLAS = ResizableAccelerationStructure("RayTracingTLAS", 50);
            
            ECS::World.observer<MeshComponent, Transform>().event(flecs::OnSet).each([&](MeshComponent& meshComponent, Transform& transform)
            {
                TLAS.AddData(meshComponent, transform);
            });
            
            ECS::World.observer<Light, Transform>().event(flecs::OnSet).each([&](Light& light, Transform& transform)
            {
                if(light.LightId == -1)
                {
                    light.LightId = RTSceneData.NumLights;
                    LightsBuffer.AddData(&light.Data, sizeof(LightData));
                    LightTransformsBuffer.AddData(&transform.Matrix, sizeof(Matrix4));
                    
                    RTSceneData.NumLights = LightsBuffer.Num();
                }
            });
            
            ECS::World.observer<Light, Transform>().event(flecs::OnSet).each([&](Light& light, Transform& transform)
            {
                if(light.LightId >= 0)
                {
                    LightsBuffer.UpdateData(&light.Data, sizeof(LightData), sizeof(LightData) * light.LightId);
                    LightTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), sizeof(Matrix4) * light.LightId);
                }
            });
            
            ECS::World.observer<Camera>().event(flecs::OnSet).each([&](Camera& camera)
            {
                RTSceneData.InvViewMatrix = inverse(camera.ViewMatrix);
                RTSceneData.InvProjectionMatrix = inverse(camera.ProjectionMatrix);
            });

            ECS::World.system<>("RayTracingSystem").kind(flecs::OnDraw).each([&]
            {
                if(IsInitialized)
                {
                    RootConstants.WorldPositionRT = WorldPositionRT->GetIndex(SRV_UAV_CBV);
                    RootConstants.NormalRT = NormalRT->GetIndex(SRV_UAV_CBV);
                    RootConstants.ColorRT = AlbedoRT->GetIndex(SRV_UAV_CBV);
                    RootConstants.ORMRT = ORMRT->GetIndex(SRV_UAV_CBV);
                    RootConstants.OutputColorRT = RadianceRT->GetIndex(SRV_UAV_CBV);
                    RootConstants.LightsBuffer = LightsBuffer.GetIndex(SRV_UAV_CBV);
                    RootConstants.LightTransformsBuffer = LightTransformsBuffer.GetIndex(SRV_UAV_CBV);
                    RootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_UAV_CBV);
                    RootConstants.TLAS = TLAS.GetIndex(SRV_UAV_CBV);
                    
                    Renderer::UploadBuffer(SceneDataBuffer, &RTSceneData, sizeof(RTSceneData));

                    //dispatching
                    Renderer::ResourceBarrier(RadianceRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::SetPipeline(RTPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(RayTracingRootConstants));
                    Renderer::TraceRays(RTPipeline, Point3(RadianceRT->GetWidth(), RadianceRT->GetHeight(), 1));
                    Renderer::ResourceBarrier(RadianceRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
                    
            IsInitialized = true;
        }
    };
}
