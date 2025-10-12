#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Texture.h"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Components/ParticleSystemComponent.h"
#include "Waldem/Renderer/GBuffer.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Waldem/Utils/ImageUtils.h"

namespace Waldem
{
    struct GizmoRootConstants
    {
        uint WorldTransforms;
        uint MaterialAttributes;
        uint SceneDataBuffer;
        uint MeshIDRTID;
    };

    struct GizmoSceneData
    {
        Matrix4 ViewMatrix;
        Matrix4 ProjectionMatrix;
        Matrix4 WorldMatrix;
        Matrix4 InverseProjectionMatrix;
    };

    enum GizmoType : uint
    {
        GizmoCamera = 0,
        GizmoSun = 1,
        GizmoPointLight = 2,
        GizmoSpotLight = 3,
        GizmoParticleSystem = 4,
    };

    class GizmosRenderingSystem : public ISystem
    {
    private:
        // Quad geometry
        WArray<Vertex> GizmoVertices;
        WArray<uint32> GizmoIndices;
        Buffer* GizmoVB = nullptr;
        Buffer* GizmoIB = nullptr;

        // Gizmo textures
        Texture2D* CameraGizmo = nullptr;
        Texture2D* SunGizmo  = nullptr;
        Texture2D* PointLightGizmo  = nullptr;
        Texture2D* SpotLightGizmo  = nullptr;
        Texture2D* ParticleSystemGizmo  = nullptr;

        // Buffers
        ResizableBuffer IndirectBuffer;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer MaterialBuffer;
        Buffer* SceneDataBuffer = nullptr;

        GizmoSceneData SceneData;

        // Materials (camera/light only)
        WArray<MaterialShaderAttribute> MaterialAttributes;

        // One indirect command per gizmo
        WArray<IndirectCommand> IndirectCommands;

        Pipeline* PipelineState = nullptr;
        PixelShader* GizmoPixelShader = nullptr;

        CImageImporter ImageImporter;

    public:
        GizmosRenderingSystem()
        {
            IndirectBuffer = ResizableBuffer("GizmoIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), 128);
            WorldTransformsBuffer = ResizableBuffer("GizmoTransforms", BufferType::StorageBuffer, sizeof(Matrix4), 128);
            MaterialBuffer = ResizableBuffer("GizmoMaterials", BufferType::StorageBuffer, sizeof(MaterialShaderAttribute), 2);
            SceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(GizmoSceneData), sizeof(GizmoSceneData));

            GizmoVertices =
            {
                { {Vector4(-0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {}, {}, {}, {Vector2(0,1)} },
                { {Vector4( 0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {}, {}, {}, {Vector2(1,1)} },
                { {Vector4( 0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {}, {}, {}, {Vector2(1,0)} },
                { {Vector4(-0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {}, {}, {}, {Vector2(0,0)} },
            };
            GizmoIndices = { 0,1,2, 2,3,0 };

            GizmoVB = Renderer::CreateBuffer("GizmoVB", BufferType::VertexBuffer, GizmoVertices.GetSize(), sizeof(Vertex), GizmoVertices.GetData());
            GizmoIB = Renderer::CreateBuffer("GizmoIB", BufferType::IndexBuffer, GizmoIndices.GetSize(), sizeof(uint32), GizmoIndices.GetData());
        }

        void Initialize(InputManager* inputManager) override
        {
            // Load gizmo icons
            CameraGizmo = ImageUtils::LoadTexture("Icons/Camera");
            SunGizmo = ImageUtils::LoadTexture("Icons/Sun");
            PointLightGizmo = ImageUtils::LoadTexture("Icons/PointLight");
            SpotLightGizmo = ImageUtils::LoadTexture("Icons/SpotLight");
            ParticleSystemGizmo = ImageUtils::LoadTexture("Icons/ParticleSystem");

            // Pipeline state
            DepthStencilDesc noDepth = DEFAULT_DEPTH_STENCIL_DESC;
            noDepth.DepthEnable = false;
            noDepth.DepthWriteMask = WD_DEPTH_WRITE_MASK_ZERO;

            RasterizerDesc rasterizer = DEFAULT_RASTERIZER_DESC;
            rasterizer.CullMode = WD_CULL_MODE_NONE;

            GizmoPixelShader = Renderer::LoadPixelShader("Gizmo");
            auto format = ViewportManager::GetEditorViewport()->FrameBuffer->GetRenderTarget(0)->GetFormat();
            PipelineState = Renderer::CreateGraphicPipeline("GizmoPipeline",
                                                            GizmoPixelShader,
                                                            { format },
                                                            TextureFormat::D32_FLOAT,
                                                            rasterizer,
                                                            noDepth,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            MaterialShaderAttribute camMat{};
            camMat.Albedo = Vector4(1,1,1,1);
            camMat.DiffuseTextureID = CameraGizmo->GetIndex(SRV_CBV);
            MaterialAttributes.Add(camMat);

            MaterialShaderAttribute sunMat{};
            sunMat.Albedo = Vector4(1,1,1,1);
            sunMat.DiffuseTextureID = SunGizmo->GetIndex(SRV_CBV);
            MaterialAttributes.Add(sunMat);

            MaterialShaderAttribute pointLightMat{};
            pointLightMat.Albedo = Vector4(1,1,1,1);
            pointLightMat.DiffuseTextureID = PointLightGizmo->GetIndex(SRV_CBV);
            MaterialAttributes.Add(pointLightMat);

            MaterialShaderAttribute spotLightMat{};
            spotLightMat.Albedo = Vector4(1,1,1,1);
            spotLightMat.DiffuseTextureID = SpotLightGizmo->GetIndex(SRV_CBV);
            MaterialAttributes.Add(spotLightMat);

            MaterialShaderAttribute particleSystemMat{};
            particleSystemMat.Albedo = Vector4(1,1,1,1);
            particleSystemMat.DiffuseTextureID = ParticleSystemGizmo->GetIndex(SRV_CBV);
            MaterialAttributes.Add(particleSystemMat);

            MaterialBuffer.UpdateOrAdd(MaterialAttributes.GetData(), MaterialAttributes.GetSize(), 0);

            // ECS observers
            ECS::World.observer<Camera, Transform>().without<EditorComponent>().event(flecs::OnAdd).each([&](flecs::entity e, Camera&, Transform& t)
            {
                AddIcon(e, t, GizmoCamera);
            });

            ECS::World.observer<Light, Transform>().event(flecs::OnAdd).each([&](flecs::entity e, Light&, Transform& t)
            {
                AddIcon(e, t, GizmoSun);
            });

            ECS::World.observer<ParticleSystemComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity e, ParticleSystemComponent&, Transform& t)
            {
                AddIcon(e, t, GizmoParticleSystem);
            });

            ECS::World.observer<Light>().event(flecs::OnSet).each([&](flecs::entity e, Light& light)
            {
                ChangeLightIcon(e, light);
            });

            ECS::World.observer<Camera>().without<EditorComponent>().event(flecs::OnRemove).each([&](flecs::entity e, Camera&)
            {
                RemoveIcon(e);
            });

            ECS::World.observer<Light>().event(flecs::OnRemove).each([&](flecs::entity e, Light&)
            {
                RemoveIcon(e);
            });

            ECS::World.observer<ParticleSystemComponent>().event(flecs::OnRemove).each([&](flecs::entity e, ParticleSystemComponent&)
            {
                RemoveIcon(e);
            });

            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity e, Transform& t)
            {
                int drawId;
                if (IdManager::GetId(e, GizmoDrawIdType, drawId))
                {
                    WorldTransformsBuffer.UpdateData(&t.Matrix, sizeof(Matrix4), drawId * sizeof(Matrix4));
                }
            });

            ECS::World.system("GizmoDrawSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                DrawIcons();
            });
        }

        void AddIcon(flecs::entity e, Transform& t, GizmoType type)
        {
            int gizmoId = IdManager::AddId(e, GizmoDrawIdType);

            WorldTransformsBuffer.UpdateOrAdd(&t.Matrix, sizeof(Matrix4), gizmoId * sizeof(Matrix4));

            if(gizmoId >= IndirectCommands.Num())
            {
                IndirectCommands.Add(IndirectCommand());
            }

            // Pack transformId + gizmo type
            uint packed = ((uint)gizmoId << 16) | (uint)type;
            
            IndirectCommand& cmd = IndirectCommands[gizmoId];
            cmd.DrawId = packed;
            cmd.DrawIndexed = { (uint)GizmoIndices.Num(), 1, 0, 0, 0 };
            IndirectBuffer.UpdateOrAdd(&cmd, sizeof(IndirectCommand), gizmoId * sizeof(IndirectCommand));
        }

        void ChangeLightIcon(flecs::entity e, Light& light)
        {
            int lightEntityId;
            if (IdManager::GetId(e, GizmoDrawIdType, lightEntityId))
            {
                GizmoType gizmoType;
                
                switch(light.Data.Type)
                {
                    case LightType::Directional:
                        gizmoType = GizmoSun;
                        break;
                    case LightType::Point:
                        gizmoType = GizmoPointLight;
                        break;
                    case LightType::Spot:
                        gizmoType = GizmoSpotLight;
                        break;
                default:
                    gizmoType = GizmoPointLight;
                    break;
                }
                
                IndirectCommand& cmd = IndirectCommands[lightEntityId];
                cmd.DrawId = ((uint)lightEntityId << 16) | (uint)gizmoType;

                IndirectBuffer.UpdateData(&cmd, sizeof(IndirectCommand), lightEntityId * sizeof(IndirectCommand));
            }
        }

        void RemoveIcon(flecs::entity e)
        {
            int gizmoId;
            if (IdManager::GetId(e, GizmoDrawIdType, gizmoId))
            {
                auto& drawCommand = IndirectCommands[gizmoId];
                drawCommand.DrawId = (uint)-1;
                IndirectBuffer.UpdateData(&drawCommand, sizeof(IndirectCommand), gizmoId * sizeof(IndirectCommand));

                IdManager::RemoveId(e, GizmoDrawIdType);
            }
        }

        void DrawIcons()
        {
            if (IndirectCommands.Num() == 0)
                return;

            auto viewport = Renderer::GetCurrentViewport();
            if(viewport != ViewportManager::GetEditorViewport())
                return;

            ECS::Entity linkedCamera;
            if(viewport->TryGetLinkedCamera(linkedCamera))
            {
                auto gbuffer = viewport->GetGBuffer();
                auto& camera = linkedCamera.get<Camera>();
                auto& transform = linkedCamera.get<Transform>();
                auto targetRT = viewport->FrameBuffer->GetCurrentRenderTarget();
                auto meshIdRT = gbuffer->GetRenderTarget(MeshID);

                SceneData.ProjectionMatrix = camera.ProjectionMatrix;
                SceneData.ViewMatrix = inverse(transform.Matrix);
                SceneData.WorldMatrix = transform.Matrix;
                SceneData.InverseProjectionMatrix = inverse(camera.ProjectionMatrix);
                Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(GizmoSceneData));

                Renderer::SetPipeline(PipelineState);
                Renderer::BindRenderTargets({ targetRT });
                Renderer::BindDepthStencil(nullptr);

                Renderer::SetVertexBuffers(GizmoVB, 1);
                Renderer::SetIndexBuffer(GizmoIB);

                GizmoRootConstants rc;
                rc.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_CBV);
                rc.MaterialAttributes = MaterialBuffer.GetIndex(SRV_CBV);
                rc.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);
                rc.MeshIDRTID = meshIdRT->GetIndex(SRV_CBV);
                Renderer::PushConstants(&rc, sizeof(GizmoRootConstants));

                gbuffer->Barriers({ Deferred, MeshID }, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::DrawIndirect(IndirectCommands.Num(), IndirectBuffer);
                gbuffer->Barriers({ Deferred, MeshID }, RENDER_TARGET, ALL_SHADER_RESOURCE);
            }
        }
    };
}
