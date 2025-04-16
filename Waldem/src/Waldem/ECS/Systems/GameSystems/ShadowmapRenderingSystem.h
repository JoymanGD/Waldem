#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API ShadowmapRenderingSystem : ISystem
    {
        //AverageWorldPosition
        // RenderTarget* WorldPositionRT = nullptr;
        // RenderTarget* DepthRT = nullptr;
        // Pipeline* AverageWorldPositionPipeline = nullptr;
        // RootSignature* AverageWorldPositionRootSignature = nullptr;
        // ComputeShader* AverageWorldPositionShader = nullptr;
        // Vector3 AverageWorldPosition;
        // Point3 GroupCount;

        //ShadowmapRendering
        Pipeline* ShadowmapRenderingPipeline = nullptr;
        RootSignature* ShadowmapRenderingRootSignature = nullptr;
        PixelShader* ShadowmapRenderingShader = nullptr;
        ResourceManager* resourceManager;
        
    public:
        ShadowmapRenderingSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            this->resourceManager = resourceManager;
            
            WArray<Matrix4> worldTransforms;
            
            for (auto [entity, mesh, transform] : Manager->EntitiesWith<MeshComponent, Transform>())
            {
                worldTransforms.Add(transform.Matrix);
            }

            WArray<Resource> resources;
            
            // Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            // WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            // DepthRT = resourceManager->GetRenderTarget("DepthRT");
            //
            // resources.Add(Resource("WorldPositionRT", WorldPositionRT, 0));
            // resources.Add(Resource("DepthRT", DepthRT, 1));
            // resources.Add(Resource("AverageWorldPositions", RTYPE_RWBuffer, nullptr, sizeof(Vector3), sizeof(Vector3), 0));
            //
            // AverageWorldPositionShader = Renderer::LoadComputeShader("AverageWorldPosition");
            // AverageWorldPositionRootSignature = Renderer::CreateRootSignature(resources);
            // AverageWorldPositionPipeline = Renderer::CreateComputePipeline("AverageWorldPositionPipeline", AverageWorldPositionRootSignature, AverageWorldPositionShader);
            // Point3 numThreads = Renderer::GetNumThreadsPerGroup(AverageWorldPositionShader);
            // GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);

            resources.Clear();
            resources.Add(Resource("MyConstantBuffer", RTYPE_ConstantBuffer, nullptr, sizeof(Matrix4), sizeof(Matrix4) * 2, 0));
            resources.Add(Resource("RootConstants", RTYPE_Constant, nullptr, sizeof(uint32_t), sizeof(uint32_t), 1));
            resources.Add(Resource("WorldTransforms", RTYPE_Buffer, worldTransforms.GetData(), sizeof(Matrix4), worldTransforms.GetSize(), 0));

            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, TextureFormat::R32G32B32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT", 0, TextureFormat::R32G32B32_FLOAT, 0, 24, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "BITANGENT", 0, TextureFormat::R32G32B32_FLOAT, 0, 36, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 48, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "MESH_ID", 0, TextureFormat::R16_UINT, 0, 56, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            ShadowmapRenderingShader = Renderer::LoadPixelShader("Shadowmap");
            ShadowmapRenderingRootSignature = Renderer::CreateRootSignature(resources);
            ShadowmapRenderingPipeline = Renderer::CreateGraphicPipeline("ShadowmapRenderingPipeline",
                                                                        ShadowmapRenderingRootSignature,
                                                                        ShadowmapRenderingShader,
                                                                        { TextureFormat::R8G8B8A8_UNORM },
                                                                        DEFAULT_RASTERIZER_DESC,
                                                                        DEFAULT_DEPTH_STENCIL_DESC,
                                                                        WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                                        DEFAULT_INPUT_LAYOUT_DESC);
        }

        void Update(float deltaTime) override
        {
            for (auto [entity, light, lightTransform] : Manager->EntitiesWith<Light, Transform>())
            {
                if(light.Data.Type == LightType::Directional)
                {
                    for (auto [cameraEntity, camera, cameraTransform, mainCamera] : Manager->EntitiesWith<Camera, Transform, EditorCamera>())
                    {
                        auto currentPosition = cameraTransform.Position;
                        currentPosition.y = lightTransform.Position.y;
                        lightTransform.SetPosition(currentPosition);
                        break;
                    }
                    
                    //Average world position computation
                    // Renderer::SetPipeline(AverageWorldPositionPipeline);
                    // Renderer::SetRootSignature(AverageWorldPositionRootSignature);
                    // Renderer::Compute(GroupCount);
                    // AverageWorldPositionRootSignature->ReadbackResourceData("AverageWorldPositions", &AverageWorldPosition);
                    //
                    // WD_CORE_INFO("AverageWorldPosition: {0}, {1}, {2}", AverageWorldPosition.x, AverageWorldPosition.y, AverageWorldPosition.z);
                    // // currentPosition.y = lightTransform.GetPosition().y;
                    //
                    // for (auto [transformEntity, transform, selected] : ECSManager->EntitiesWith<Transform, Selected>())
                    // {
                    //     transform.SetPosition(AverageWorldPosition);
                    //     break;
                    // }
                    
                    Matrix4 matrices[2];
                    matrices[0] = lightTransform.Inverse();
                    matrices[1] = light.Data.Projection;

                    //Create frustrum for culling
                    Frustrum frustrum;
                    auto frustrumPlanes = frustrum.GetPlanes(matrices[1] * matrices[0]);
                
                    ShadowmapRenderingRootSignature->UpdateResourceData("MyConstantBuffer", matrices);

                    WArray<Matrix4> worldTransforms;
                    for (auto [modelEntity, mesh, modelTransform] : Manager->EntitiesWith<MeshComponent, Transform>())
                    {
                        worldTransforms.Add(modelTransform.Matrix);
                    }

                    ShadowmapRenderingRootSignature->UpdateResourceData("WorldTransforms", worldTransforms.GetData());
                    
                    Renderer::SetPipeline(ShadowmapRenderingPipeline);
                    Renderer::SetRootSignature(ShadowmapRenderingRootSignature);
                    Renderer::ResourceBarrier(light.Shadowmap, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    Renderer::SetRenderTargets({ nullptr }, light.Shadowmap, { (float)light.Shadowmap->GetWidth(), (float)light.Shadowmap->GetHeight() }, { 0, 0, light.Shadowmap->GetWidth(), light.Shadowmap->GetHeight() });
                    Renderer::ClearDepthStencil(light.Shadowmap);

                    uint32_t meshID = 0;

                    for (auto [modelEtity, meshComponent, modelTransform] : Manager->EntitiesWith<MeshComponent, Transform>())
                    {
                        ShadowmapRenderingRootSignature->UpdateResourceData("RootConstants", &meshID);
                        
                        auto transformedBBox = meshComponent.Mesh->BBox.GetTransformed(modelTransform.Matrix);

                        //Frustrum culling
                        if(transformedBBox.IsInFrustum(frustrumPlanes))
                        {
                            Renderer::Draw(meshComponent.Mesh);
                        }

                        meshID++;
                    }
                    
                    Renderer::ResourceBarrier(light.Shadowmap, DEPTH_WRITE, ALL_SHADER_RESOURCE);

                    Renderer::SetRenderTargets({});
                }
            }
        }
    };
}
