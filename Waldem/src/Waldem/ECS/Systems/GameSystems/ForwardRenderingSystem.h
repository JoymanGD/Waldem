#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API ForwardRenderingSystem : ISystem
    {
        Pipeline* DefaultPipeline = nullptr;
        RootSignature* DefaultRootSignature = nullptr;
        PixelShader* DefaultPixelShader = nullptr;
        RenderTarget* DefaultRenderTarget = nullptr;
        
    public:
        ForwardRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            RenderTarget* testShadowMap = nullptr;
            WArray<LightShaderData> LightDatas;
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                testShadowMap = light.Shadowmap;
                LightDatas.Add(lightData);
            }
            
            WArray<Texture2D*> textures;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                for (auto texture : model.Model->GetTextures())
                    textures.Add(texture);

                break;
            }

            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform.GetMatrix());
            }
            
            WArray<Resource> resources;
            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            resources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            if(!LightDatas.IsEmpty())
                resources.Add(Resource("LightsBuffer", RTYPE_Buffer, nullptr, sizeof(LightShaderData), LightDatas.GetSize(), 0));
            if(testShadowMap)
                resources.Add(Resource("Shadowmap", testShadowMap, 1));
            if(!worldTransforms.IsEmpty())
                resources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 2));
            if(!textures.IsEmpty())
                resources.Add(Resource("TestTextures", textures, 3));
            resources.Add(Resource("ComparisonSampler", { Sampler( COMPARISON_MIN_MAG_MIP_LINEAR, WRAP, WRAP, WRAP, LESS_EQUAL) }, 1));
        
            DefaultRenderTarget = Renderer::CreateRenderTarget("DefaultRenderTarget", sceneData->Window->GetWidth(), sceneData->Window->GetHeight(), TextureFormat::R8G8B8A8_UNORM);
            DefaultRootSignature = Renderer::CreateRootSignature(resources);
            DefaultPixelShader = Renderer::LoadPixelShader("ForwardRendering");
            DefaultPipeline = Renderer::CreateGraphicPipeline("ForwardRenderingPipeline",
                                                            DefaultRootSignature,
                                                            DefaultPixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);
        }

        void Update(float deltaTime) override
        {
            WArray<LightShaderData> LightDatas;
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                LightShaderData lightData(light.Data, transform);
                LightDatas.Add(lightData);
            }

            if(!LightDatas.IsEmpty())
                DefaultRootSignature->UpdateResourceData("LightsBuffer", LightDatas.GetData());

            WArray<FrustumPlane> frustrumPlanes;
            Matrix4 matrices[2];
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, MainCamera, Transform>())
            {
                matrices[0] = camera.ViewMatrix;
                matrices[1] = camera.ProjectionMatrix;
                DefaultRootSignature->UpdateResourceData("MyConstantBuffer", matrices);
                frustrumPlanes = camera.ExtractFrustumPlanes();

                break;
            }

            //collect world transforms for buffer update
            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform);
            }

            DefaultRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
            Renderer::SetPipeline(DefaultPipeline);
            Renderer::SetRootSignature(DefaultRootSignature);
            
            uint32_t modelID = 0;
            for (auto [entity, modelComponent, transform] : ECSManager->EntitiesWith<ModelComponent, Transform>())
            {
                DefaultRootSignature->UpdateResourceData("RootConstants", &modelID);
                for (auto mesh : modelComponent.Model->GetMeshes())
                {
                    auto transformedBBox = mesh->BBox.GetTransformed(transform.Matrix);

                    //Frustrum culling
                    if(transformedBBox.IsInFrustum(frustrumPlanes))
                    {
                        Renderer::Draw(mesh);
                    }
                }
                modelID++;
            }
        }
    };
}
