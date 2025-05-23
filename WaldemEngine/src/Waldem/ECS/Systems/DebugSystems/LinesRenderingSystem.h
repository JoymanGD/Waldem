#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Renderer/Model/Line.h"
#include "Waldem/Renderer/Model/LineMesh.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"

namespace Waldem
{
    class WALDEM_API LinesRenderingSystem : public ISystem
    {
        Pipeline* LinePipeline = nullptr;
        PixelShader* LinePixelShader = nullptr;

        LineMesh LMesh = {};
        
    public:
        LinesRenderingSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthStencilDesc.DepthEnable = false;
            
            LinePixelShader = Renderer::LoadPixelShader("Line");
            LinePipeline = Renderer::CreateGraphicPipeline("LinePipeline",
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
            
            for (auto [entity, camera, mainCamera, cameraTransform] : Manager->EntitiesWith<Camera, EditorCamera, Transform>())
            {
                viewProj = camera.ProjectionMatrix * camera.ViewMatrix;
                break;
            }
            
            WArray<Line> lines;

            for (auto [transformEntity, transform, collider, meshComponent] : Manager->EntitiesWith<Transform, ColliderComponent, MeshComponent>())
            {
                Vector4 color = collider.IsColliding ? Vector4(1.0f, 0.0f, 0.0f, 1.0f) : Vector4(0.0f, 1.0f, 0.0f, 1.0f);
                lines.AddRange(meshComponent.Mesh->BBox.GetTransformed(transform).GetLines(color));
            }

            Renderer::UpdateGraphicResource(LMesh.VertexBuffer, lines.GetData(), sizeof(Line) * lines.Num());
            Renderer::SetPipeline(LinePipeline);
            Renderer::PushConstants(&viewProj, sizeof(Matrix4));
            Renderer::Draw(&LMesh);
        }
    };
}
