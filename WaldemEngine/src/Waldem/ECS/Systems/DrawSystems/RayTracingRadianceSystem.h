#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/GraphicResource.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Light.h"

namespace Waldem
{
    struct RayTracingSceneData
    {
        Matrix4 InvViewMatrix;
        Matrix4 InvProjectionMatrix;
        int NumLights;
    };

    struct RayTracingRootConstants
    {
        uint WorldPositionRT;
        uint NormalRT;
        uint ColorRT;
        uint ORMRT;
        uint ShadowsRT;
        uint LightsBuffer;
        uint LightTransformsBuffer;
        uint SceneDataBuffer; 
        uint TLAS;
    };
    
    class WALDEM_API RayTracingRadianceSystem : public DrawSystem
    {
        RenderTarget* RadianceRT = nullptr;
        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        WArray<AccelerationStructure*> BLAS;
        WMap<CMesh*, AccelerationStructure*> BLASToUpdate;
        AccelerationStructure* TLAS = nullptr;
        RayTracingSceneData RTSceneData;
        WArray<RayTracingInstance> Instances;
        Buffer* LightsBuffer = nullptr;
        Buffer* LightTransformsBuffer = nullptr;
        Buffer* SceneDataBuffer = nullptr;
        RayTracingRootConstants RootConstants;
        
    public:
        RayTracingRadianceSystem(ECSManager* eCSManager) : DrawSystem(eCSManager) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            if(Manager->EntitiesWith<MeshComponent, Transform>().Count() <= 0)
                return;
            
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            auto worldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            auto normalRT = resourceManager->GetRenderTarget("NormalRT");
            auto albedoRT = resourceManager->GetRenderTarget("ColorRT");
            auto ormRT = resourceManager->GetRenderTarget("ORMRT");
            
            WArray<LightData> LightDatas;
            WArray<Matrix4> LightTransforms;
            for (auto [entity, light, transform] : Manager->EntitiesWith<Light, Transform>())
            {
                LightDatas.Add(light.Data);
                LightTransforms.Add(transform);
            }
            
            for (auto [transformEntity, transform, meshComponent] : Manager->EntitiesWith<Transform, MeshComponent>())
            {
                WArray geometries { RayTracingGeometry(meshComponent.Mesh->VertexBuffer, meshComponent.Mesh->IndexBuffer) };
                
                AccelerationStructure* blas = Renderer::CreateBLAS(meshComponent.Mesh->Name, geometries);
                BLAS.Add(blas);

                Instances.Add(RayTracingInstance(blas, transform));

                if(transformEntity.Has<Ocean>())
                {
                    BLASToUpdate.Add(meshComponent.Mesh, blas);
                }
            }
            
            TLAS = Renderer::CreateTLAS("RayTracingTLAS", Instances);
            LightsBuffer = Renderer::CreateBuffer("LightsBuffer", StorageBuffer, &LightDatas[0], LightDatas.GetSize(), sizeof(LightData));
            LightTransformsBuffer = Renderer::CreateBuffer("LightTransformsBuffer", StorageBuffer, &LightTransforms[0], LightTransforms.GetSize(), sizeof(Matrix4));
            SceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, &RTSceneData, sizeof(RTSceneData), sizeof(RTSceneData));

            RootConstants.WorldPositionRT = worldPositionRT->GetIndex(SRV_UAV_CBV);
            RootConstants.NormalRT = normalRT->GetIndex(SRV_UAV_CBV);
            RootConstants.ColorRT = albedoRT->GetIndex(SRV_UAV_CBV);
            RootConstants.ORMRT = ormRT->GetIndex(SRV_UAV_CBV);
            RootConstants.ShadowsRT = RadianceRT->GetIndex(SRV_UAV_CBV);
            RootConstants.LightsBuffer = LightsBuffer->GetIndex(SRV_UAV_CBV);
            RootConstants.LightTransformsBuffer = LightTransformsBuffer->GetIndex(SRV_UAV_CBV);
            RootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_UAV_CBV);
            RootConstants.TLAS = TLAS->GetIndex(SRV_UAV_CBV);
            
            RTShader = Renderer::LoadRayTracingShader("RayTracing/Radiance");
            RTPipeline = Renderer::CreateRayTracingPipeline("PostProcessPipeline", RTShader);

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(RTShader) RTShader->Destroy();
            if(RTPipeline) RTPipeline->Destroy();
            if(TLAS) Renderer::Destroy(TLAS);

            for (auto blas : BLAS)
            {
                if(blas) Renderer::Destroy(blas);
            }

            BLAS.Clear();
            Instances.Clear();
            BLASToUpdate.Clear();
            
            IsInitialized = false;
        }

        void Update(float deltaTime) override
        {
            if(!IsInitialized)
                return;
            
            //lights update
            WArray<LightData> LightDatas;
            WArray<Matrix4> LightTransforms;
            for (auto [entity, light, transform] : Manager->EntitiesWith<Light, Transform>())
            {
                LightDatas.Add(light.Data);
                LightTransforms.Add(transform);
            }
            if(!LightDatas.IsEmpty())
                Renderer::UpdateGraphicResource(LightsBuffer, LightDatas.GetData(), LightDatas.GetSize());
            if(!LightTransforms.IsEmpty())
                Renderer::UpdateGraphicResource(LightTransformsBuffer, LightTransforms.GetData(), LightTransforms.GetSize());
            
            //constant buffer update
            for (auto [entity, camera, mainCamera, cameraTransform] : Manager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                RTSceneData.InvViewMatrix = inverse(camera.ViewMatrix);
                RTSceneData.InvProjectionMatrix = inverse(camera.ProjectionMatrix);

                break;
            }

            int i = 0;
            for (auto [entity, meshComponent, transform] : Manager->EntitiesWith<MeshComponent, Transform>())
            {
                Instances[i].Transform = transform;
                i++;
            }

            for (auto blas : BLASToUpdate)
            {
                WArray geometries { RayTracingGeometry(blas.key->VertexBuffer, blas.key->IndexBuffer) };
                Renderer::UpdateBLAS(blas.value, geometries);
            }

            Renderer::UpdateTLAS(TLAS, Instances);
            
            RTSceneData.NumLights = LightDatas.Num();
            
            Renderer::UpdateGraphicResource(SceneDataBuffer, &RTSceneData, sizeof(RTSceneData));

            //dispatching
            Renderer::ResourceBarrier(RadianceRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::SetPipeline(RTPipeline);
            Renderer::PushConstants(&RootConstants, sizeof(RayTracingRootConstants));
            Renderer::TraceRays(RTPipeline, Point3(RadianceRT->GetWidth(), RadianceRT->GetHeight(), 1));
            Renderer::ResourceBarrier(RadianceRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }

        void OnResize(Vector2 size) override
        {
            
        }
    };
}