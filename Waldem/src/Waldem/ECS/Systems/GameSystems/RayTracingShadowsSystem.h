#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/RootSignature.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct RayTracingSceneData
    {
        Matrix4 InvViewMatrix;
        Matrix4 InvProjectionMatrix;
        int NumLights;
    };
    
    class WALDEM_API RayTracingShadowsSystem : ISystem
    {
        RenderTarget* RadianceRT = nullptr;
        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        RootSignature* RTRootSignature = nullptr;
        WArray<AccelerationStructure*> BLAS;
        AccelerationStructure* TLAS = nullptr;
        RayTracingSceneData RTSceneData;
        
    public:
        RayTracingShadowsSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            // Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            auto worldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            auto normalRT = resourceManager->GetRenderTarget("NormalRT");
            auto albedoRT = resourceManager->GetRenderTarget("ColorRT");
            auto ormRT = resourceManager->GetRenderTarget("ORMRT");
            
            WArray<Light> LightDatas;
            WArray<Matrix4> LightTransforms;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightDatas.Add(light);
                LightTransforms.Add(transform);
            }

            WArray<RayTracingInstance> instances;
            
            for (auto [transformEntity, transform, meshComponent] : ECSManager->EntitiesWith<Transform, MeshComponent>())
            {
                WArray geometries { RayTracingGeometry(meshComponent.Mesh->VertexBuffer, meshComponent.Mesh->IndexBuffer) };
                
                AccelerationStructure* blas = Renderer::CreateBLAS(meshComponent.Mesh->Name, geometries);
                BLAS.Add(blas);

                instances.Add(RayTracingInstance(blas, transform));
            }
            
            TLAS = Renderer::CreateTLAS("RayTracingTLAS", instances);
            
            WArray<Resource> rtResources;
            rtResources.Add(Resource("TLAS", TLAS, 0));
            rtResources.Add(Resource("LightsBuffer", RTYPE_Buffer, nullptr, sizeof(Light), LightDatas.GetSize(), 1));
            rtResources.Add(Resource("LightTransforms", RTYPE_Buffer, nullptr, sizeof(Matrix4), LightTransforms.GetSize(), 2));
            rtResources.Add(Resource("WorldPositionRT", worldPositionRT, 3));
            rtResources.Add(Resource("NormalRT", normalRT, 4));
            rtResources.Add(Resource("ColorRT", albedoRT, 5));
            rtResources.Add(Resource("ORMRT", ormRT, 6));
            rtResources.Add(Resource("ShadowsRT", RadianceRT, 0, true));
            rtResources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(RayTracingSceneData), sizeof(RayTracingSceneData), 0)); 
            RTRootSignature = Renderer::CreateRootSignature(rtResources);
            RTShader = Renderer::LoadRayTracingShader("RayTracing/TestRayTracingShader");
            RTPipeline = Renderer::CreateRayTracingPipeline("PostProcessPipeline", RTRootSignature, RTShader);
        }

        void Update(float deltaTime) override
        {
            //lights update
            WArray<Light> LightDatas;
            WArray<Matrix4> LightTransforms;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightDatas.Add(light);
                LightTransforms.Add(transform);
            }
            if(!LightDatas.IsEmpty())
                RTRootSignature->UpdateResourceData("LightsBuffer", LightDatas.GetData());
            if(!LightTransforms.IsEmpty())
                RTRootSignature->UpdateResourceData("LightTransforms", LightTransforms.GetData());
            
            //constant buffer update
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                RTSceneData.InvViewMatrix = inverse(camera.ViewMatrix);
                RTSceneData.InvProjectionMatrix = inverse(camera.ProjectionMatrix);

                break;
            }
            
            RTSceneData.NumLights = LightDatas.Num();
            
            RTRootSignature->UpdateResourceData("MyConstantBuffer", &RTSceneData);

            //dispatching
            Renderer::ResourceBarrier(RadianceRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
            Renderer::SetPipeline(RTPipeline);
            Renderer::SetRootSignature(RTRootSignature);
            Renderer::TraceRays(RTPipeline, Point3(RadianceRT->GetWidth(), RadianceRT->GetHeight(), 1));
            Renderer::ResourceBarrier(RadianceRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
        }
    };
}