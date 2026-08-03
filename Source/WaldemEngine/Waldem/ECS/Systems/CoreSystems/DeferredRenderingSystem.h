#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Coach/TinyCuda/NIV/NIVCoach.h"
#include <algorithm>
#include <vector>

namespace Waldem
{
    struct DeferredRootConstants
    {
        uint MeshIDRT;
        uint RadianceRT;
        uint PathTracingRT;
        uint ColorRT;
        uint DeferredRT;
        uint NIVIrradianceRT;
        uint NIVPredictedBuffer;
        uint SkyColorRT;
        uint EnableSky;
        uint EnablePathTracing;
        uint EnableNIVInference;
        uint HasNIVPrediction;
        uint EnableNIVSpatialFilter;
        float NIVSpatialFilterStrength;
    };

    struct NIVPackRootConstants
    {
        uint WorldPositionRT;
        uint WorldNormalRT;
        uint MeshIDRT;
        uint OutWorldPositionBuffer;
        uint OutWorldNormalValidBuffer;
        uint Width;
        uint Height;
    };

    class WALDEM_API DeferredRenderingSystem : public ISystem
    {
        inline static DeferredRenderingSystem* ActiveInstance = nullptr;

        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        Point3 GroupCount;
        DeferredRootConstants DeferredRootConstantsData;
        ComputeShader* NIVPackComputeShader = nullptr;
        Pipeline* NIVPackPipeline = nullptr;
        NIVPackRootConstants NIVPackRootConstantsData;
        Buffer* NIVWorldPositionBuffer = nullptr;
        Buffer* NIVWorldNormalValidBuffer = nullptr;
        Buffer* NIVPredictedOutputBuffer = nullptr;
        Buffer* NIVPredictedHistoryBuffer = nullptr;
        uint NIVBufferCapacityPixels = 0;

        void EnsureNIVBuffers(uint pixelCount)
        {
            if(pixelCount <= NIVBufferCapacityPixels)
            {
                return;
            }

            if(NIVWorldPositionBuffer)
            {
                Renderer::Destroy(NIVWorldPositionBuffer);
                delete NIVWorldPositionBuffer;
                NIVWorldPositionBuffer = nullptr;
            }

            if(NIVWorldNormalValidBuffer)
            {
                Renderer::Destroy(NIVWorldNormalValidBuffer);
                delete NIVWorldNormalValidBuffer;
                NIVWorldNormalValidBuffer = nullptr;
            }

            if(NIVPredictedOutputBuffer)
            {
                Renderer::Destroy(NIVPredictedOutputBuffer);
                delete NIVPredictedOutputBuffer;
                NIVPredictedOutputBuffer = nullptr;
            }

            if(NIVPredictedHistoryBuffer)
            {
                Renderer::Destroy(NIVPredictedHistoryBuffer);
                delete NIVPredictedHistoryBuffer;
                NIVPredictedHistoryBuffer = nullptr;
            }

            NIVWorldPositionBuffer = Renderer::CreateBuffer(
                "NIVWorldPositionBuffer",
                StorageBuffer,
                pixelCount * sizeof(float) * 4,
                sizeof(float) * 4
            );
            NIVWorldNormalValidBuffer = Renderer::CreateBuffer(
                "NIVWorldNormalValidBuffer",
                StorageBuffer,
                pixelCount * sizeof(float) * 4,
                sizeof(float) * 4
            );
            NIVPredictedOutputBuffer = Renderer::CreateBuffer(
                "NIVPredictedOutputBuffer",
                StorageBuffer,
                pixelCount * sizeof(float) * 4,
                sizeof(float) * 4
            );
            NIVPredictedHistoryBuffer = Renderer::CreateBuffer(
                "NIVPredictedHistoryBuffer",
                StorageBuffer,
                pixelCount * sizeof(float) * 4,
                sizeof(float) * 4
            );
            NIVBufferCapacityPixels = pixelCount;
        }

    public:
        DeferredRenderingSystem()
        {
            ActiveInstance = this;
        }

        static void ResetSceneRuntimeData()
        {
            if(ActiveInstance == nullptr)
            {
                return;
            }

            Renderer::RenderData.NIVLastInferenceAttempted = false;
            Renderer::RenderData.NIVLastInferenceSucceeded = false;
            Renderer::RenderData.NIVLastValidPixelCount = 0;
            Renderer::RenderData.NIVLastMeanLuminance = 0.0f;
            Renderer::RenderData.NIVLastMaxChannel = 0.0f;
        }

        void Initialize() override
        {
            AlwaysActive = true;

            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingComputeShader);
            NIVPackComputeShader = Renderer::LoadComputeShader("NIVPackInputs");
            NIVPackPipeline = Renderer::CreateComputePipeline("NIVPackPipeline", NIVPackComputeShader);
            NIVWorldPositionBuffer = Renderer::CreateBuffer("NIVWorldPositionBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVWorldNormalValidBuffer = Renderer::CreateBuffer("NIVWorldNormalValidBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVPredictedOutputBuffer = Renderer::CreateBuffer("NIVPredictedOutputBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVPredictedHistoryBuffer = Renderer::CreateBuffer("NIVPredictedHistoryBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVBufferCapacityPixels = 1;

            ECS::World.system("DeferredRenderingSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();

                ECS::Entity linkedCamera;

                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto gbuffer = viewport->GetGBuffer();

                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    gbuffer->Clear({Deferred, NIVIrradiance});

                    if(!Renderer::RenderData.FeatureToggles.EnableDeferredPass)
                    {
                        return;
                    }

                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    const uint width = (uint)resolution.x;
                    const uint height = (uint)resolution.y;
                    const uint pixelCount = width * height;
                    const uint selectedOutputTarget = viewport->Type == EditorViewport ? Renderer::RenderData.EditorViewportOutputTarget : Renderer::RenderData.GameViewportOutputTarget;

                    bool hasNIVPrediction = false;
                    const bool needNIV = Renderer::RenderData.FeatureToggles.EnableNIVInference || selectedOutputTarget == 8;
                    auto* nivCoach = reinterpret_cast<Coach::TinyCuda::NIVCoach*>(Renderer::RenderData.NIVRuntimeCoach);
                    Renderer::RenderData.NIVLastInferenceAttempted = needNIV;
                    Renderer::RenderData.NIVLastInferenceSucceeded = false;
                    Renderer::RenderData.NIVLastValidPixelCount = 0;
                    Renderer::RenderData.NIVLastMeanLuminance = 0.0f;
                    Renderer::RenderData.NIVLastMaxChannel = 0.0f;

                    if(needNIV && nivCoach && pixelCount > 0)
                    {
                        try
                        {
                            EnsureNIVBuffers(pixelCount);

                            NIVPackRootConstantsData.WorldPositionRT = gbuffer->GetRenderTarget(WorldPosition)->GetIndex(SRV_CBV);
                            NIVPackRootConstantsData.WorldNormalRT = gbuffer->GetRenderTarget(Normal)->GetIndex(SRV_CBV);
                            NIVPackRootConstantsData.MeshIDRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                            NIVPackRootConstantsData.OutWorldPositionBuffer = NIVWorldPositionBuffer->GetIndex(UAV);
                            NIVPackRootConstantsData.OutWorldNormalValidBuffer = NIVWorldNormalValidBuffer->GetIndex(UAV);
                            NIVPackRootConstantsData.Width = width;
                            NIVPackRootConstantsData.Height = height;

                            Renderer::ResourceBarrier(NIVWorldPositionBuffer, UNORDERED_ACCESS);
                            Renderer::ResourceBarrier(NIVWorldNormalValidBuffer, UNORDERED_ACCESS);
                            Renderer::SetPipeline(NIVPackPipeline);
                            Renderer::PushConstants(&NIVPackRootConstantsData, sizeof(NIVPackRootConstantsData));
                            Point3 nivPackThreads = Renderer::GetNumThreadsPerGroup(NIVPackComputeShader);
                            Point3 nivPackGroupCount = Point3((width + nivPackThreads.x - 1) / nivPackThreads.x, (height + nivPackThreads.y - 1) / nivPackThreads.y, 1);
                            Renderer::Compute(nivPackGroupCount);
                            Renderer::ResourceBarrier(NIVWorldPositionBuffer, ALL_SHADER_RESOURCE);
                            Renderer::ResourceBarrier(NIVWorldNormalValidBuffer, ALL_SHADER_RESOURCE);

                            Renderer::Flush();

                            uint validPixels = 0;
                            float meanLuminance = 0.0f;
                            float maxChannel = 0.0f;
                            const bool interopSucceeded = nivCoach->InferFromSharedBuffers(
                                Renderer::GetSharedHandle(NIVWorldPositionBuffer),
                                pixelCount * sizeof(float) * 4,
                                Renderer::GetSharedHandle(NIVWorldNormalValidBuffer),
                                pixelCount * sizeof(float) * 4,
                                Renderer::GetSharedHandle(NIVPredictedOutputBuffer),
                                pixelCount * sizeof(float) * 4,
                                Renderer::GetSharedHandle(NIVPredictedHistoryBuffer),
                                pixelCount * sizeof(float) * 4,
                                pixelCount,
                                Renderer::RenderData.EnableNIVTemporalSmoothing,
                                Renderer::RenderData.NIVTemporalHistoryWeight,
                                &validPixels,
                                &meanLuminance,
                                &maxChannel
                            );

                            if(interopSucceeded)
                            {
                                NIVPredictedOutputBuffer->SetCurrentState(ALL_SHADER_RESOURCE);
                                if(NIVPredictedHistoryBuffer)
                                {
                                    NIVPredictedHistoryBuffer->SetCurrentState(ALL_SHADER_RESOURCE);
                                }

                                Renderer::RenderData.NIVLastInferenceSucceeded = true;
                                Renderer::RenderData.NIVLastValidPixelCount = validPixels;
                                Renderer::RenderData.NIVLastMeanLuminance = meanLuminance;
                                Renderer::RenderData.NIVLastMaxChannel = maxChannel;
                                hasNIVPrediction = true;
                            }
                            else
                            {
                                std::vector<float> worldPositionRGBA(pixelCount * 4, 0.0f);
                                std::vector<float> worldNormalValidRGBA(pixelCount * 4, 0.0f);
                                std::vector<float> predictedRGBA(pixelCount * 4, 0.0f);
                                Renderer::DownloadBuffer(NIVWorldPositionBuffer, worldPositionRGBA.data(), worldPositionRGBA.size() * sizeof(float));
                                Renderer::DownloadBuffer(NIVWorldNormalValidBuffer, worldNormalValidRGBA.data(), worldNormalValidRGBA.size() * sizeof(float));
                                nivCoach->InferFromBuffers(worldPositionRGBA.data(), worldNormalValidRGBA.data(), pixelCount, predictedRGBA.data());
                                Renderer::UploadBuffer(NIVPredictedOutputBuffer, predictedRGBA.data(), (uint32_t)(predictedRGBA.size() * sizeof(float)), 0);
                                if(NIVPredictedOutputBuffer->GetCurrentState() != ALL_SHADER_RESOURCE)
                                {
                                    Renderer::ResourceBarrier(NIVPredictedOutputBuffer, ALL_SHADER_RESOURCE);
                                }

                                double lumAccum = 0.0;
                                for(uint i = 0; i < pixelCount; ++i)
                                {
                                    const float valid = worldNormalValidRGBA[i * 4 + 3];
                                    if(valid <= 0.5f)
                                    {
                                        continue;
                                    }
                                    validPixels++;
                                    const float r = predictedRGBA[i * 4 + 0];
                                    const float g = predictedRGBA[i * 4 + 1];
                                    const float b = predictedRGBA[i * 4 + 2];
                                    lumAccum += (double)r * 0.2126 + (double)g * 0.7152 + (double)b * 0.0722;
                                    maxChannel = std::max(maxChannel, std::max(r, std::max(g, b)));
                                }
                                Renderer::RenderData.NIVLastInferenceSucceeded = true;
                                Renderer::RenderData.NIVLastValidPixelCount = validPixels;
                                Renderer::RenderData.NIVLastMeanLuminance = validPixels > 0 ? (float)(lumAccum / (double)validPixels) : 0.0f;
                                Renderer::RenderData.NIVLastMaxChannel = maxChannel;
                                hasNIVPrediction = true;
                            }
                        }
                        catch(...)
                        {
                            hasNIVPrediction = false;
                        }
                    }

                    gbuffer->Barriers({Deferred, NIVIrradiance}, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);

                    DeferredRootConstantsData.MeshIDRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                    DeferredRootConstantsData.RadianceRT = gbuffer->GetRenderTarget(Radiance)->GetIndex(SRV_CBV);
                    DeferredRootConstantsData.PathTracingRT = gbuffer->GetRenderTarget(PathTracing)->GetIndex(SRV_CBV);
                    DeferredRootConstantsData.ColorRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                    DeferredRootConstantsData.DeferredRT = deferredRT->GetIndex(UAV);
                    DeferredRootConstantsData.NIVIrradianceRT = gbuffer->GetRenderTarget(NIVIrradiance)->GetIndex(UAV);
                    DeferredRootConstantsData.NIVPredictedBuffer = NIVPredictedOutputBuffer ? NIVPredictedOutputBuffer->GetIndex(SRV_CBV) : 0;
                    DeferredRootConstantsData.SkyColorRT = gbuffer->GetRenderTarget(SkyColor)->GetIndex(SRV_CBV);
                    DeferredRootConstantsData.EnableSky = Renderer::RenderData.FeatureToggles.EnableSkyPass ? 1 : 0;
                    DeferredRootConstantsData.EnablePathTracing = Renderer::RenderData.FeatureToggles.EnablePathTracing ? 1 : 0;
                    DeferredRootConstantsData.EnableNIVInference = Renderer::RenderData.FeatureToggles.EnableNIVInference ? 1 : 0;
                    DeferredRootConstantsData.HasNIVPrediction = hasNIVPrediction ? 1 : 0;
                    DeferredRootConstantsData.EnableNIVSpatialFilter = Renderer::RenderData.EnableNIVSpatialFilter ? 1 : 0;
                    DeferredRootConstantsData.NIVSpatialFilterStrength = std::clamp(Renderer::RenderData.NIVSpatialFilterStrength, 0.0f, 1.0f);
                    Renderer::SetPipeline(DeferredRenderingPipeline);
                    Renderer::PushConstants(&DeferredRootConstantsData, sizeof(DeferredRootConstantsData));

                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);

                    Renderer::Compute(GroupCount);
                    viewport->GetGBuffer()->Barriers({Deferred, NIVIrradiance}, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}
