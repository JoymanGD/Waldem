#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/BoxMesh.h"
#include "Waldem/Renderer/Viewport/Viewport.h"

namespace Waldem
{
    class WALDEM_API BoundingBoxRenderingSystem : public ISystem
    {
        Pipeline* BoundingBoxPipeline = nullptr;
        PixelShader* LinePixelShader = nullptr;
        WMap<ECS::Entity, BoxMesh> BoxMeshes;
        bool IsInitialized = false;

    public:
        BoundingBoxRenderingSystem() {}

        void Initialize(InputManager* inputManager) override
        {
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthStencilDesc.DepthWriteMask = WD_DEPTH_WRITE_MASK_ZERO;
            depthStencilDesc.DepthFunc = WD_COMPARISON_FUNC_LESS_EQUAL;

            LinePixelShader = Renderer::LoadPixelShader("Line");
            BoundingBoxPipeline = Renderer::CreateGraphicPipeline("BoundingBoxPipeline",
                                                                  LinePixelShader,
                                                                  { TextureFormat::R8G8B8A8_UNORM },
                                                                  TextureFormat::D32_FLOAT,
                                                                  DEFAULT_RASTERIZER_DESC,
                                                                  depthStencilDesc,
                                                                  DEFAULT_BLEND_DESC,
                                                                  WD_PRIMITIVE_TOPOLOGY_TYPE_LINE,
                                                                  inputElementDescs);

            ECS::World.observer<Transform, AABB>().event(flecs::OnAdd).each([&](flecs::entity entity, Transform& transform, AABB& bbox)
            {
                BoxMeshes[entity] = BoxMesh();

                if(entity.has<MeshComponent>())
                {
                    auto& meshComp = entity.get_mut<MeshComponent>();
                    if(meshComp.MeshRef.IsValid())
                    {
                        bbox = meshComp.MeshRef.Mesh->BBox;
                    }
                }
                
                Vector4 color = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
                auto boxLines = bbox.GetTransformed(transform).GetLines(color);
                
                Renderer::UploadBuffer(BoxMeshes[entity].VertexBuffer, boxLines.GetData(), boxLines.GetSize());
            });

            ECS::World.observer<Transform, AABB>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform, AABB& bbox)
            {
                if(BoxMeshes.Contains(entity))
                {
                    Vector4 color = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
                    auto boxLines = bbox.GetTransformed(transform).GetLines(color);
                    
                    Renderer::UploadBuffer(BoxMeshes[entity].VertexBuffer, boxLines.GetData(), boxLines.GetSize());
                }
            });

            ECS::World.observer<AABB>().event(flecs::OnRemove).each([&](flecs::entity entity, AABB& bbox)
            {
                if(BoxMeshes.Contains(entity))
                {
                    Renderer::Destroy(BoxMeshes[entity].VertexBuffer);
                    BoxMeshes.Remove(entity);
                }
            });

            ECS::World.system("BoundingBoxRenderingSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                ECS::Entity linkedCamera;
                if (viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto& camera = linkedCamera.get_mut<Camera>();
                    auto gbuffer = viewport->GetGBuffer();

                    gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_READ);
                    
                    Renderer::SetPipeline(BoundingBoxPipeline);
                    Renderer::PushConstants(&camera.ViewProjectionMatrix, sizeof(Matrix4));

                    for (auto& lineMeshPair : BoxMeshes)
                    {
                        Renderer::BindRenderTargets(viewport->FrameBuffer->GetCurrentRenderTarget());
                        Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                        Renderer::SetVertexBuffers(lineMeshPair.value.VertexBuffer, 1);
                        Renderer::Draw(&lineMeshPair.value);
                    }
                    
                    gbuffer->Barrier(Depth, DEPTH_READ, ALL_SHADER_RESOURCE);
                }
            });

            IsInitialized = true;
        }
    };
}
