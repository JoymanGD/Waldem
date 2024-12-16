#pragma once
#include "System.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API ShadowmapRenderingSystem : ISystem
    {
        Pipeline* DefaultPipeline = nullptr;
        RootSignature* DefaultRootSignature = nullptr;
        PixelShader* DefaultShadowmappingShader = nullptr;
        
    public:
        ShadowmapRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override
        {
            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.GetMatrix());
            }
            
            WArray<Resource> resources;

            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            resources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            if(!worldTransforms.IsEmpty())
                resources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));
            
            DefaultShadowmappingShader = Renderer::LoadPixelShader("Shadowmap");
            DefaultRootSignature = Renderer::CreateRootSignature(resources);
            DefaultPipeline = Renderer::CreateGraphicPipeline("ForwardRenderingPipeline", { TextureFormat::R8G8B8A8_UNORM }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, DefaultRootSignature, DefaultShadowmappingShader);
        }

        void Update(SceneData* sceneData, float deltaTime) override
        {
            for (auto [entity, light, lightTransform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                if(light.Data.Type == LightType::Directional)
                {
                    for (auto [cameraEntity, camera, cameraTransform, mainCamera] : ECSManager->EntitiesWith<Camera, Transform, MainCamera>())
                    {
                        auto currentPosition = cameraTransform.GetPosition();
                        currentPosition.y = lightTransform.GetPosition().y;
                        lightTransform.SetPosition(currentPosition);
                        break;
                    }

                    Matrix4 matrices[2];
                    matrices[0] = lightTransform.Inverse();
                    matrices[1] = light.Data.Projection;

                    //Create frustrum for culling
                    Frustrum frustrum;
                    auto frustrumPlanes = frustrum.GetPlanes(matrices[1] * matrices[0]);
                
                    DefaultRootSignature->UpdateResourceData("MyConstantBuffer", matrices);

                    WArray<Matrix4> worldTransforms;
                    for (auto [modelEntity, modelComponent, modelTransform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
                    {
                        worldTransforms.Add(modelTransform.GetMatrix());
                    }

                    DefaultRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
                    
                    Renderer::SetPipeline(DefaultPipeline);
                    Renderer::SetRootSignature(DefaultRootSignature);
                    Renderer::ResourceBarrier(light.Shadowmap, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    Renderer::SetRenderTargets({}, light.Shadowmap);
                    Renderer::ClearDepthStencil(light.Shadowmap);

                    uint32_t modelID = 0;

                    for (auto [modelEtity, modelComponent, modelTransform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
                    {
                        DefaultRootSignature->UpdateResourceData("RootConstants", &modelID);
                        
                        for (auto mesh : modelComponent.Model->GetMeshes())
                        {
                            auto transformedBBox = mesh->BBox.Transform(modelTransform.GetMatrix());

                            //Frustrum culling
                            if(transformedBBox.IsInFrustum(frustrumPlanes))
                            {
                                Renderer::Draw(mesh);
                            }
                        }

                        modelID++;
                    }
                    
                    Renderer::ResourceBarrier(light.Shadowmap, DEPTH_WRITE, ALL_SHADER_RESOURCE);

                    Renderer::SetRenderTargets({});
                }
            }
        }
    };
}
