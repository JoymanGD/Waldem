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
        WArray<Line> Lines;
        Matrix4 ViewProjection;
        
    public:
        LinesRenderingSystem() {}

        void Initialize(InputManager* inputManager) override
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
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            depthStencilDesc,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_LINE,
                                                            inputElementDescs);
            
            ECS::World.observer<Transform, MeshComponent, ColliderComponent>().event(flecs::OnSet).each([&](Transform& transform, MeshComponent& meshComponent, ColliderComponent& collider)
            {
                Vector4 color = collider.IsColliding ? Vector4(1.0f, 0.0f, 0.0f, 1.0f) : Vector4(0.0f, 1.0f, 0.0f, 1.0f);
                Lines.AddRange(((CMesh*)meshComponent.MeshRef.Mesh)->BBox.GetTransformed(transform).GetLines(color));
            });

            ECS::World.system("LineRenderingSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                if(IsInitialized)
                {
                    if(auto editorCamera = ECS::World.lookup("EditorCamera"))
                    {
                        ViewProjection = editorCamera.get<Camera>().ViewProjectionMatrix;
                    }
                    
                    Renderer::UploadBuffer(LMesh.VertexBuffer, Lines.GetData(), sizeof(Line) * Lines.Num());
                    Renderer::SetPipeline(LinePipeline);
                    Renderer::PushConstants(&ViewProjection, sizeof(Matrix4));
                    Renderer::Draw(&LMesh);
                }
            });
                    
            IsInitialized = true;
        }
    };
}
