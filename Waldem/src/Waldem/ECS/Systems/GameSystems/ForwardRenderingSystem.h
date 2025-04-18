#pragma once
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/RootSignature.h"

namespace Waldem
{
    class WALDEM_API ForwardRenderingSystem : ISystem
    {
        Pipeline* DefaultPipeline = nullptr;
        RootSignature* DefaultRootSignature = nullptr;
        PixelShader* DefaultPixelShader = nullptr;
        RenderTarget* DefaultRenderTarget = nullptr;
        RenderTarget* RadianceRT = nullptr;
        
    public:
        ForwardRenderingSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            
            WArray<Texture2D*> textures;
            for (auto [entity, model, transform] : Manager->EntitiesWith<ModelComponent, Transform>())
            {
                for (auto texture : model.Model->GetTextures())
                    textures.Add(texture);

                break;
            }

            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : Manager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform);
            }
            
            WArray<Resource> resources;
            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            resources.Add(Resource("RootConstants", RTYPE_Constant, 1, nullptr, 1));
            resources.Add(Resource("RadianceRT", RadianceRT, 0));
            if(!worldTransforms.IsEmpty())
                resources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 1));
            if(!textures.IsEmpty())
                resources.Add(Resource("TestTextures", textures, 2));
        
            
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
            WArray<FrustumPlane> frustrumPlanes;
            Matrix4 matrices[2];
            for (auto [entity, camera, mainCamera, cameraTransform] : Manager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                matrices[0] = camera.ViewMatrix;
                matrices[1] = camera.ProjectionMatrix;
                DefaultRootSignature->UpdateResourceData("MyConstantBuffer", matrices);
                frustrumPlanes = camera.ExtractFrustumPlanes();

                break;
            }

            //collect world transforms for buffer update
            WArray<Matrix4> worldTransforms;
            for (auto [entity, model, transform] : Manager->EntitiesWith<ModelComponent, Transform>())
            {
                worldTransforms.Add(transform);
            }

            DefaultRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
            Renderer::SetPipeline(DefaultPipeline);
            Renderer::SetRootSignature(DefaultRootSignature);
            
            uint32_t modelID = 0;
            for (auto [entity, modelComponent, transform] : Manager->EntitiesWith<ModelComponent, Transform>())
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
