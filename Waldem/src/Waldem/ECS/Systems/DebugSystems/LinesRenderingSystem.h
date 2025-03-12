#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Model/Line.h"
#include "Waldem/Renderer/Model/LineMesh.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API LinesRenderingSystem : ISystem
    {
        Pipeline* LinePipeline = nullptr;
        RootSignature* LineRootSignature = nullptr;
        PixelShader* LinePixelShader = nullptr;

        LineMesh LMesh = {};
        
    public:
        LinesRenderingSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            WArray<Resource> resources;
            resources.Add(Resource("RootConstants", RTYPE_Constant, nullptr, sizeof(Matrix4), sizeof(Matrix4), 0));
            
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthStencilDesc.DepthEnable = false;
            
            LineRootSignature = Renderer::CreateRootSignature(resources);
            LinePixelShader = Renderer::LoadPixelShader("Line");
            LinePipeline = Renderer::CreateGraphicPipeline("LinePipeline",
                                                            LineRootSignature,
                                                            LinePixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            DEFAULT_RASTERIZER_DESC,
                                                            depthStencilDesc,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_LINE,
                                                            inputElementDescs);
        }

        void Update(float deltaTime) override
        {
            Matrix4 viewProj;
            
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, MainCamera, Transform>())
            {
                viewProj = camera.ProjectionMatrix * camera.ViewMatrix;
                break;
            }
            
            WArray<Line> lines;

            for (auto [transformEntity, transform, physicsComponent, meshComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent, MeshComponent>())
            {
                lines.AddRange(meshComponent.Mesh->BBox.GetTransformed(transform).GetLines(Vector4(0.0f, 1.0f, 0.0f, 1.0f)));
            }

            Renderer::UpdateBuffer(LMesh.VertexBuffer, lines.GetData(), sizeof(Line) * lines.Num());
            Renderer::SetPipeline(LinePipeline);
            Renderer::SetRootSignature(LineRootSignature);
            LineRootSignature->UpdateResourceData("RootConstants", &viewProj);
            Renderer::Draw(&LMesh);
        }
    };
}
