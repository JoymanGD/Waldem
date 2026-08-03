#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include <cstring>

namespace Waldem
{
    struct SRayTracingSceneData
    {
        Matrix4 InvViewMatrix;
        Matrix4 InvProjectionMatrix;
        Vector3 CameraPosition;
        int NumLights = 0;
    };

    struct RayTracingRootConstants
    {
        uint WorldPositionRT;
        uint NormalRT;
        uint ColorRT;
        uint ORMRT;
        uint RadianceRT;
        uint PathTracingRT;
        uint ReflectionRT;
        uint LightsBuffer;
        uint LightTransformsBuffer;
        uint LightsIndicesBuffer;
        uint SceneDataBuffer;
        uint TLAS;
        uint VertexBuffer;
        uint IndexBuffer;
        uint DrawCommandsBuffer;
        uint MaterialBuffer;
        uint EnableReflections;
        uint EnableDirectLighting;
        uint EnableSpecular;
        uint EnableMetallic;
        uint EnablePathTracing;
        uint PathTracingMaxBounces;
        uint PathTracingSamplesPerPixel;
        uint PathTracingFrameIndex;
        uint EnablePathTracingAccumulation;
    };

    class WALDEM_API RayTracingRenderingSystem : public ISystem
    {
        inline static RayTracingRenderingSystem* ActiveInstance = nullptr;

        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        Pipeline* PathTracingPipeline = nullptr;
        RayTracingShader* PathTracingShader = nullptr;
        SRayTracingSceneData RayTracingSceneData;
        Buffer* RayTracingSceneDataBuffer = nullptr;
        RayTracingRootConstants RayTracingRootConstantsData;
        uint PathTracingFrameIndex = 0;
        bool PathTracingHistoryValid = false;
        Matrix4 LastPathTracingInvView = Matrix4(1.0f);
        Matrix4 LastPathTracingInvProjection = Matrix4(1.0f);
        Vector3 LastPathTracingCameraPosition = Vector3(0.0f);

    public:
        RayTracingRenderingSystem()
        {
            ActiveInstance = this;
        }

        static void ResetSceneRuntimeData()
        {
            if(ActiveInstance == nullptr)
            {
                return;
            }

            auto& instance = *ActiveInstance;
            instance.PathTracingFrameIndex = 0;
            instance.PathTracingHistoryValid = false;
            instance.LastPathTracingInvView = Matrix4(1.0f);
            instance.LastPathTracingInvProjection = Matrix4(1.0f);
            instance.LastPathTracingCameraPosition = Vector3(0.0f);
        }

        void Initialize() override
        {
            AlwaysActive = true;

            RTShader = Renderer::LoadRayTracingShader("RayTracing/Radiance");
            RTPipeline = Renderer::CreateRayTracingPipeline("RayTracingPipeline", RTShader);
            PathTracingShader = Renderer::LoadRayTracingShader("RayTracing/PathTracing");
            PathTracingPipeline = Renderer::CreateRayTracingPipeline("PathTracingPipeline", PathTracingShader);
            RayTracingSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, sizeof(SRayTracingSceneData), sizeof(SRayTracingSceneData), &RayTracingSceneData);

            ECS::World.system("RayTracingRenderingSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();

                ECS::Entity linkedCamera;
                if(!viewport->TryGetLinkedCamera(linkedCamera))
                {
                    return;
                }

                auto gbuffer = viewport->GetGBuffer();
                auto radianceRT = gbuffer->GetRenderTarget(Radiance);

                if(!Renderer::RenderData.FeatureToggles.EnableRayTracingPass)
                {
                    gbuffer->Clear({ Radiance, Reflection });
                    return;
                }

                if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                {
                    return;
                }

                auto& renderData = Renderer::RenderData;
                if(!renderData.SharedDrawCommandsBuffer || !renderData.SharedMaterialAttributesBuffer)
                {
                    return;
                }

                auto& cameraComponent = linkedCamera.get<Camera>();
                auto& transformComponent = linkedCamera.get<Transform>();

                RayTracingSceneData.CameraPosition = transformComponent.RenderPosition;
                RayTracingSceneData.InvViewMatrix = transformComponent.RenderMatrix;
                RayTracingSceneData.InvProjectionMatrix = inverse(cameraComponent.ProjectionMatrix);
                RayTracingSceneData.NumLights = static_cast<int>(renderData.SharedNumLights);
                Renderer::UploadBuffer(RayTracingSceneDataBuffer, &RayTracingSceneData, sizeof(SRayTracingSceneData));

                auto& toggles = renderData.FeatureToggles;
                bool cameraChanged =
                    !PathTracingHistoryValid ||
                    memcmp(&LastPathTracingInvView, &RayTracingSceneData.InvViewMatrix, sizeof(Matrix4)) != 0 ||
                    memcmp(&LastPathTracingInvProjection, &RayTracingSceneData.InvProjectionMatrix, sizeof(Matrix4)) != 0 ||
                    memcmp(&LastPathTracingCameraPosition, &RayTracingSceneData.CameraPosition, sizeof(Vector3)) != 0;

                if(toggles.EnablePathTracing)
                {
                    if(cameraChanged || !toggles.EnablePathTracingAccumulation)
                    {
                        PathTracingFrameIndex = 0;
                    }
                    else
                    {
                        PathTracingFrameIndex++;
                    }
                }
                else
                {
                    PathTracingFrameIndex = 0;
                }

                PathTracingHistoryValid = true;
                LastPathTracingInvView = RayTracingSceneData.InvViewMatrix;
                LastPathTracingInvProjection = RayTracingSceneData.InvProjectionMatrix;
                LastPathTracingCameraPosition = RayTracingSceneData.CameraPosition;

                RayTracingRootConstantsData.WorldPositionRT = gbuffer->GetRenderTarget(WorldPosition)->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.NormalRT = gbuffer->GetRenderTarget(Normal)->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.ColorRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.ORMRT = gbuffer->GetRenderTarget(ORM)->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.RadianceRT = radianceRT->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.PathTracingRT = gbuffer->GetRenderTarget(PathTracing)->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.ReflectionRT = gbuffer->GetRenderTarget(Reflection)->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.LightsBuffer = renderData.SharedLightsBuffer ? renderData.SharedLightsBuffer->GetIndex(SRV_CBV) : 0;
                RayTracingRootConstantsData.LightTransformsBuffer = renderData.SharedLightTransformsBuffer ? renderData.SharedLightTransformsBuffer->GetIndex(SRV_CBV) : 0;
                RayTracingRootConstantsData.LightsIndicesBuffer = renderData.SharedLightsIndicesBuffer ? renderData.SharedLightsIndicesBuffer->GetIndex(SRV_CBV) : 0;
                RayTracingRootConstantsData.SceneDataBuffer = RayTracingSceneDataBuffer->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.TLAS = renderData.TLAS.GetIndex(SRV_CBV);
                RayTracingRootConstantsData.VertexBuffer = renderData.VertexBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstantsData.IndexBuffer = renderData.IndexBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstantsData.DrawCommandsBuffer = renderData.SharedDrawCommandsBuffer->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.MaterialBuffer = renderData.SharedMaterialAttributesBuffer->GetIndex(SRV_CBV);
                RayTracingRootConstantsData.EnableReflections = toggles.EnableReflections ? 1 : 0;
                RayTracingRootConstantsData.EnableDirectLighting = toggles.EnableDirectLighting ? 1 : 0;
                RayTracingRootConstantsData.EnableSpecular = toggles.EnableSpecular ? 1 : 0;
                RayTracingRootConstantsData.EnableMetallic = toggles.EnableMetallic ? 1 : 0;
                RayTracingRootConstantsData.EnablePathTracing = toggles.EnablePathTracing ? 1 : 0;
                RayTracingRootConstantsData.PathTracingMaxBounces = toggles.PathTracingMaxBounces > 4 ? 4 : toggles.PathTracingMaxBounces;
                RayTracingRootConstantsData.PathTracingSamplesPerPixel = toggles.PathTracingSamplesPerPixel;
                RayTracingRootConstantsData.PathTracingFrameIndex = PathTracingFrameIndex;
                RayTracingRootConstantsData.EnablePathTracingAccumulation = toggles.EnablePathTracingAccumulation ? 1 : 0;

                gbuffer->Barriers({Radiance, PathTracing, Reflection}, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                Pipeline* activeRTPipeline = toggles.EnablePathTracing ? PathTracingPipeline : RTPipeline;
                Renderer::SetPipeline(activeRTPipeline);
                Renderer::PushConstants(&RayTracingRootConstantsData, sizeof(RayTracingRootConstantsData));
                Renderer::TraceRays(activeRTPipeline, Point3(radianceRT->GetWidth(), radianceRT->GetHeight(), 1));
                gbuffer->Barriers({Radiance, PathTracing, Reflection}, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            });
        }
    };
}
