#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/BoxMesh.h"
#include "Waldem/Renderer/Viewport/Viewport.h"

namespace Waldem
{
    class WALDEM_API CollisionRenderingSystem : public ISystem
    {
        Pipeline* CollisionPipeline = nullptr;
        PixelShader* LineShader = nullptr;
        WMap<ECS::Entity, BoxMesh> ColliderMeshes;
        bool IsInitialized = false;

    public:
        CollisionRenderingSystem() {}

        void Initialize(InputManager* inputManager) override
        {
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            RasterizerDesc rasterizerDesc = DEFAULT_RASTERIZER_DESC;
            rasterizerDesc.FillMode = WD_FILL_MODE_WIREFRAME;
            rasterizerDesc.CullMode = WD_CULL_MODE_NONE;

            DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthStencilDesc.DepthWriteMask = WD_DEPTH_WRITE_MASK_ZERO;
            depthStencilDesc.DepthFunc = WD_COMPARISON_FUNC_LESS_EQUAL;

            LineShader = Renderer::LoadPixelShader("Line");

            CollisionPipeline = Renderer::CreateGraphicPipeline(
                "CollisionRenderingPipeline",
                LineShader,
                { TextureFormat::R8G8B8A8_UNORM },
                TextureFormat::D32_FLOAT,
                rasterizerDesc,
                depthStencilDesc,
                DEFAULT_BLEND_DESC,
                WD_PRIMITIVE_TOPOLOGY_TYPE_LINE,
                inputElementDescs
            );

            ECS::World.observer<Transform, ColliderComponent>().event(flecs::OnAdd).each([&](flecs::entity entity, Transform& transform, ColliderComponent& collider)
            {
                if (collider.Type != Box)
                    return;

                ColliderMeshes[entity] = BoxMesh();
                auto& boxMesh = ColliderMeshes[entity];

                Vector4 color = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
                auto boxLines = GetBoxColliderLines(collider.BoxSize, collider.BoxOffset, transform.Matrix, color);

                Renderer::UploadBuffer(boxMesh.VertexBuffer, boxLines.GetData(), boxLines.GetSize());
            });

            ECS::World.observer<Transform, ColliderComponent>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform, ColliderComponent& collider)
            {
                if (!ColliderMeshes.Contains(entity) || collider.Type != ColliderType::Box)
                    return;

                Vector4 color = collider.IsColliding ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 1);

                auto boxLines = GetBoxColliderLines(collider.BoxSize, collider.BoxOffset, transform.Matrix, color);
                Renderer::UploadBuffer(ColliderMeshes[entity].VertexBuffer, boxLines.GetData(), boxLines.GetSize());
            });

            ECS::World.observer<ColliderComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, ColliderComponent& collider)
            {
                if (ColliderMeshes.Contains(entity))
                {
                    Renderer::Destroy(ColliderMeshes[entity].VertexBuffer);
                    ColliderMeshes.Remove(entity);
                }
            });

            ECS::World.system("CollisionRenderingSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                ECS::Entity linkedCamera;
                if (!viewport->TryGetLinkedCamera(linkedCamera))
                    return;

                auto& camera = linkedCamera.get_mut<Camera>();
                auto gbuffer = viewport->GetGBuffer();

                gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_READ);

                Renderer::SetPipeline(CollisionPipeline);
                Renderer::PushConstants(&camera.ViewProjectionMatrix, sizeof(Matrix4));

                for (auto& pair : ColliderMeshes)
                {
                    Renderer::BindRenderTargets(viewport->FrameBuffer->GetCurrentRenderTarget());
                    Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                    Renderer::SetVertexBuffers(pair.value.VertexBuffer, 1);
                    Renderer::Draw(&pair.value);
                }

                gbuffer->Barrier(Depth, DEPTH_READ, ALL_SHADER_RESOURCE);
            });

            IsInitialized = true;
        }

        WArray<Line> GetBoxColliderLines(Vector3 size, Vector3 offset, Matrix4 transform, Vector4 color)
        {
            WArray<Line> lines;
            Vector3 h = size * 0.5f;

            Vector3 corners[8] = {
                { -h.x, -h.y, -h.z },
                {  h.x, -h.y, -h.z },
                {  h.x,  h.y, -h.z },
                { -h.x,  h.y, -h.z },
                { -h.x, -h.y,  h.z },
                {  h.x, -h.y,  h.z },
                {  h.x,  h.y,  h.z },
                { -h.x,  h.y,  h.z },
            };

            for (int i = 0; i < 8; ++i)
                corners[i] = transform * Vector4(corners[i] + offset, 1.0f);

            lines.Add({ corners[0], corners[1], color });
            lines.Add({ corners[1], corners[2], color });
            lines.Add({ corners[2], corners[3], color });
            lines.Add({ corners[3], corners[0], color });
            lines.Add({ corners[4], corners[5], color });
            lines.Add({ corners[5], corners[6], color });
            lines.Add({ corners[6], corners[7], color });
            lines.Add({ corners[7], corners[4], color });
            lines.Add({ corners[0], corners[4], color });
            lines.Add({ corners[1], corners[5], color });
            lines.Add({ corners[2], corners[6], color });
            lines.Add({ corners[3], corners[7], color });

            return lines;
        }
    };
}