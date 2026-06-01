#pragma once
#include "Waldem/ECS/Systems/CoreSystem.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/Coach/TinyCuda/NIV/NIVCoach.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Waldem/Renderer/Model/StaticMesh.h"
#include "Waldem/Types/BasicTypes.h"
#include "Waldem/Types/String.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <random>
#include <stb_image_write.h>

namespace Waldem
{
    struct TrainingPathTracingRootConstants
    {
        uint OutputBufferID;
        uint TLASID;
        uint VertexBufferId;
        uint IndexBufferId;
        uint TriangleRefsBufferId;
        uint TriangleCdfBufferId;
        uint DrawCommandsBufferId;
        uint MaterialBufferId;
        uint WorldTransformsBufferId;
        uint LightsBufferId;
        uint LightTransformsBufferId;
        uint LightsIndicesBufferId;
        uint NumTriangleRefs;
        uint NumLights;
        uint RaysPerPoint;
        uint MaxBounces;
        uint SeedOffset;
        float SceneMinX;
        float SceneMinY;
        float SceneMinZ;
        float SceneMinW;
        float SceneMaxX;
        float SceneMaxY;
        float SceneMaxZ;
        float SceneMaxW;
        float SurfaceSampleProbability;
        uint EnableBackfaceCulling;
        uint Padding0 = 0;
        uint Padding1 = 0;
    };

    struct TrainingDatasetHeader
    {
        uint32 Magic = 0x44505454; // "TTPD"
        uint32 Version = 1;
        uint32 SampleCount = 0;
        uint32 SampleStride = 0;
    };

    using TrainingSampleCPU = Coach::TinyCuda::NIVTrainingSampleGPU;

    struct TrainingTriangleRef
    {
        uint DrawId;
        uint TriangleId;
    };

    static_assert((sizeof(TrainingPathTracingRootConstants) % 4) == 0, "TrainingPathTracingRootConstants must be 32-bit aligned.");
    static_assert(sizeof(TrainingPathTracingRootConstants) <= 128, "TrainingPathTracingRootConstants exceeds push constants limit (128 bytes).");

    class WALDEM_API TrainingPathTracingSystem : public ICoreSystem
    {
        inline static TrainingPathTracingSystem* Instance = nullptr;
        RayTracingShader* TrainingShader = nullptr;
        Pipeline* TrainingPipeline = nullptr;
        Buffer* TrainingOutputBuffer = nullptr;
        Buffer* TriangleRefsBuffer = nullptr;
        Buffer* TriangleCdfBuffer = nullptr;
        uint TriangleRefsCapacityBytes = 0;
        uint TriangleCdfCapacityBytes = 0;
        uint TrainingOutputCapacityBytes = 0;
        TrainingPathTracingRootConstants RootConstants = {};
        Vector3 CachedSceneMin = Vector3(0.0f);
        Vector3 CachedSceneMax = Vector3(0.0f);
        bool HasCachedSceneBounds = false;
        bool SamplingResourcesReady = false;
        uint CachedDrawCommandsCount = 0;
        size_t CachedVertexBufferSize = 0;
        size_t CachedIndexBufferSize = 0;
        uint CachedNumTriangleRefs = 0;
        float SurfaceSampleProbability = .7f;

        void EnsureTrainingPipeline()
        {
            if(!TrainingShader)
            {
                TrainingShader = Renderer::LoadRayTracingShader("RayTracing/TrainingPathTracing");
            }

            if(!TrainingPipeline && TrainingShader)
            {
                TrainingPipeline = Renderer::CreateRayTracingPipeline("TrainingPathTracingPipeline", TrainingShader);
            }
        }

        static Path ResolveOutputPath(const WString& configuredPath)
        {
            Path outputPath = configuredPath.ToString();
            if(outputPath.is_absolute())
            {
                return outputPath;
            }

            Path contentRoot = Path(PROJECT_CONTENT_PATH);
            std::error_code ec;
            Path current = std::filesystem::current_path(ec);
            if(!ec)
            {
                auto isProjectRoot = [](const Path& candidate) -> bool
                {
                    std::error_code localEc;
                    return std::filesystem::exists(candidate / "Content", localEc) &&
                           std::filesystem::exists(candidate / "Source", localEc);
                };

                for(Path probe = current; !probe.empty(); probe = probe.parent_path())
                {
                    if(isProjectRoot(probe))
                    {
                        contentRoot = (probe / "Content").lexically_normal();
                        break;
                    }

                    if(probe == probe.root_path())
                    {
                        break;
                    }
                }
            }
            std::string generic = outputPath.generic_string();
            bool startsWithContent = generic.rfind("Content/", 0) == 0 || generic == "Content";

            if(startsWithContent)
            {
                return contentRoot.parent_path() / outputPath;
            }

            return contentRoot / outputPath;
        }

        static uint8 ToByte(float value)
        {
            if(!std::isfinite(value))
            {
                value = 0.0f;
            }
            float v = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
            return static_cast<uint8>(v * 255.0f + 0.5f);
        }

        static void SavePNG(const Path& outputPath, const std::vector<uint8>& rgb, uint width, uint height)
        {
            stbi_write_png(outputPath.string().c_str(), static_cast<int>(width), static_cast<int>(height), 3, rgb.data(), static_cast<int>(width * 3));
        }

        static std::vector<uint8> UpscaleNearest(const std::vector<uint8>& src, uint srcW, uint srcH, uint dstW, uint dstH)
        {
            std::vector<uint8> dst(static_cast<size_t>(dstW) * static_cast<size_t>(dstH) * 3, 0);
            for(uint y = 0; y < dstH; y++)
            {
                uint sy = (y * srcH) / dstH;
                for(uint x = 0; x < dstW; x++)
                {
                    uint sx = (x * srcW) / dstW;
                    size_t s = (static_cast<size_t>(sy) * srcW + sx) * 3;
                    size_t d = (static_cast<size_t>(y) * dstW + x) * 3;
                    dst[d + 0] = src[s + 0];
                    dst[d + 1] = src[s + 1];
                    dst[d + 2] = src[s + 2];
                }
            }
            return dst;
        }

        static void SaveDebugTextures(const std::vector<TrainingSampleCPU>& samples, const Path& datasetPath)
        {
            const uint sampleCount = static_cast<uint>(samples.size());
            if(sampleCount == 0)
            {
                return;
            }

            const uint width = static_cast<uint>(std::ceil(std::sqrt(static_cast<double>(sampleCount))));
            const uint height = (sampleCount + width - 1) / width;
            const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

            std::vector<uint8> worldPosRGB(pixelCount * 3, 0);
            std::vector<uint8> normalRGB(pixelCount * 3, 0);
            std::vector<uint8> irradianceRGB(pixelCount * 3, 0);

            Vector3 minPos(FLT_MAX);
            Vector3 maxPos(-FLT_MAX);
            for(const auto& s : samples)
            {
                minPos.x = std::min(minPos.x, s.WorldPosition.x);
                minPos.y = std::min(minPos.y, s.WorldPosition.y);
                minPos.z = std::min(minPos.z, s.WorldPosition.z);

                maxPos.x = std::max(maxPos.x, s.WorldPosition.x);
                maxPos.y = std::max(maxPos.y, s.WorldPosition.y);
                maxPos.z = std::max(maxPos.z, s.WorldPosition.z);
            }

            Vector3 posRange = maxPos - minPos;
            if(std::abs(posRange.x) < 1e-6f) posRange.x = 1.0f;
            if(std::abs(posRange.y) < 1e-6f) posRange.y = 1.0f;
            if(std::abs(posRange.z) < 1e-6f) posRange.z = 1.0f;

            for(uint i = 0; i < sampleCount; i++)
            {
                const auto& s = samples[i];
                const size_t idx = static_cast<size_t>(i) * 3;

                float posR = (s.WorldPosition.x - minPos.x) / posRange.x;
                float posG = (s.WorldPosition.y - minPos.y) / posRange.y;
                float posB = (s.WorldPosition.z - minPos.z) / posRange.z;
                worldPosRGB[idx + 0] = ToByte(posR);
                worldPosRGB[idx + 1] = ToByte(posG);
                worldPosRGB[idx + 2] = ToByte(posB);

                Vector3 nVec = Vector3(s.WorldNormal.x, s.WorldNormal.y, s.WorldNormal.z);
                float nLenSq = nVec.x * nVec.x + nVec.y * nVec.y + nVec.z * nVec.z;
                Vector3 n = nLenSq > 1e-12f ? normalize(nVec) : Vector3(0.0f, 1.0f, 0.0f);
                normalRGB[idx + 0] = ToByte(n.x * 0.5f + 0.5f);
                normalRGB[idx + 1] = ToByte(n.y * 0.5f + 0.5f);
                normalRGB[idx + 2] = ToByte(n.z * 0.5f + 0.5f);

                float ir = s.DiffuseIrradiance.x / (1.0f + std::max(0.0f, s.DiffuseIrradiance.x));
                float ig = s.DiffuseIrradiance.y / (1.0f + std::max(0.0f, s.DiffuseIrradiance.y));
                float ib = s.DiffuseIrradiance.z / (1.0f + std::max(0.0f, s.DiffuseIrradiance.z));
                irradianceRGB[idx + 0] = ToByte(ir);
                irradianceRGB[idx + 1] = ToByte(ig);
                irradianceRGB[idx + 2] = ToByte(ib);
            }

            const Path debugDir = datasetPath.parent_path() / "Debug";
            std::filesystem::create_directories(debugDir);

            const std::string stem = datasetPath.stem().string();
            const uint minPreviewSize = 512;
            uint previewW = width;
            uint previewH = height;
            if(previewW < minPreviewSize || previewH < minPreviewSize)
            {
                float sx = static_cast<float>(minPreviewSize) / static_cast<float>(previewW);
                float sy = static_cast<float>(minPreviewSize) / static_cast<float>(previewH);
                float s = sx > sy ? sx : sy;
                previewW = static_cast<uint>(std::ceil(previewW * s));
                previewH = static_cast<uint>(std::ceil(previewH * s));
            }

            std::vector<uint8> worldPosPreview = (previewW == width && previewH == height) ? worldPosRGB : UpscaleNearest(worldPosRGB, width, height, previewW, previewH);
            std::vector<uint8> normalPreview = (previewW == width && previewH == height) ? normalRGB : UpscaleNearest(normalRGB, width, height, previewW, previewH);
            std::vector<uint8> irradiancePreview = (previewW == width && previewH == height) ? irradianceRGB : UpscaleNearest(irradianceRGB, width, height, previewW, previewH);

            SavePNG(debugDir / (stem + "_worldpos.png"), worldPosPreview, previewW, previewH);
            SavePNG(debugDir / (stem + "_normal.png"), normalPreview, previewW, previewH);
            SavePNG(debugDir / (stem + "_irradiance.png"), irradiancePreview, previewW, previewH);
        }

    public:
        TrainingPathTracingSystem() { Instance = this; }

        static TrainingPathTracingSystem* Get() { return Instance; }

        bool GetSceneBounds(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ) const
        {
            if(!HasCachedSceneBounds)
            {
                return false;
            }

            minX = CachedSceneMin.x;
            minY = CachedSceneMin.y;
            minZ = CachedSceneMin.z;
            maxX = CachedSceneMax.x;
            maxY = CachedSceneMax.y;
            maxZ = CachedSceneMax.z;
            return true;
        }

        bool GenerateLiveNIVBatchShared(uint requestedSampleCount, void*& sharedHandle, size_t& sizeBytes, uint32& sampleStride)
        {
            sharedHandle = nullptr;
            sizeBytes = 0;
            sampleStride = sizeof(TrainingSampleCPU);

            if(requestedSampleCount == 0)
            {
                return false;
            }

            EnsureTrainingPipeline();
            if(!TrainingPipeline)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live shared batch generation failed because TrainingPipeline is not initialized.");
                return false;
            }

            auto& renderData = Renderer::RenderData;
            if(!renderData.SharedDrawCommandsBuffer || !renderData.SharedMaterialAttributesBuffer || !renderData.SharedWorldTransformsBuffer || renderData.SharedDrawCommandsCount == 0)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live shared batch generation failed because scene buffers are not ready.");
                return false;
            }

            if(renderData.SharedNumLights == 0u)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live shared batch generation failed because scene has no lights.");
                return false;
            }

            if(renderData.SharedNumLights > 0 &&
               (!renderData.SharedLightsBuffer || !renderData.SharedLightTransformsBuffer || !renderData.SharedLightsIndicesBuffer))
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live shared batch generation failed because light buffers are not ready.");
                return false;
            }

            const uint drawCommandsCount = renderData.SharedDrawCommandsCount;
            const bool mustRebuildSamplingResources =
                !SamplingResourcesReady ||
                CachedDrawCommandsCount != drawCommandsCount ||
                CachedVertexBufferSize != renderData.VertexBuffer.Size ||
                CachedIndexBufferSize != renderData.IndexBuffer.Size;

            if(mustRebuildSamplingResources)
            {
                std::vector<DrawIndexedCommand> drawCommandsCPU(drawCommandsCount);
                Renderer::DownloadBuffer(renderData.SharedDrawCommandsBuffer, drawCommandsCPU.data(), drawCommandsCount * sizeof(DrawIndexedCommand));

                std::vector<Matrix4> worldTransformsCPU(drawCommandsCount);
                Renderer::DownloadBuffer(renderData.SharedWorldTransformsBuffer, worldTransformsCPU.data(), drawCommandsCount * sizeof(Matrix4));

                uint vertexCount = renderData.VertexBuffer.Size / sizeof(Vertex);
                uint indexCount = renderData.IndexBuffer.Size / sizeof(uint);
                if(vertexCount == 0 || indexCount == 0)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: live shared batch generation failed because vertex/index buffers are empty.");
                    return false;
                }

                std::vector<Vertex> verticesCPU(vertexCount);
                std::vector<uint> indicesCPU(indexCount);
                Renderer::DownloadBuffer(renderData.VertexBuffer.GetBuffer(), verticesCPU.data(), renderData.VertexBuffer.Size);
                Renderer::DownloadBuffer(renderData.IndexBuffer.GetBuffer(), indicesCPU.data(), renderData.IndexBuffer.Size);

                std::vector<TrainingTriangleRef> triangleRefs;
                std::vector<float> triangleCdf;
                triangleRefs.reserve(65536);
                triangleCdf.reserve(65536);
                Vector3 sceneMin(FLT_MAX, FLT_MAX, FLT_MAX);
                Vector3 sceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

                double totalArea = 0.0;
                for(uint drawId = 0; drawId < drawCommandsCount; drawId++)
                {
                    const auto& command = drawCommandsCPU[drawId];
                    if(command.IndexCountPerInstance < 3)
                    {
                        continue;
                    }

                    uint triCount = command.IndexCountPerInstance / 3;
                    for(uint tri = 0; tri < triCount; tri++)
                    {
                        uint i0Idx = command.StartIndexLocation + tri * 3 + 0;
                        uint i1Idx = command.StartIndexLocation + tri * 3 + 1;
                        uint i2Idx = command.StartIndexLocation + tri * 3 + 2;
                        if(i2Idx >= indexCount)
                        {
                            continue;
                        }

                        int baseVertex = command.BaseVertexLocation;
                        int vi0 = static_cast<int>(indicesCPU[i0Idx]) + baseVertex;
                        int vi1 = static_cast<int>(indicesCPU[i1Idx]) + baseVertex;
                        int vi2 = static_cast<int>(indicesCPU[i2Idx]) + baseVertex;
                        if(vi0 < 0 || vi1 < 0 || vi2 < 0 || static_cast<uint>(vi0) >= vertexCount || static_cast<uint>(vi1) >= vertexCount || static_cast<uint>(vi2) >= vertexCount)
                        {
                            continue;
                        }

                        const Matrix4& world = worldTransformsCPU[drawId];
                        Vector4 wp0 = world * verticesCPU[vi0].Position;
                        Vector4 wp1 = world * verticesCPU[vi1].Position;
                        Vector4 wp2 = world * verticesCPU[vi2].Position;
                        Vector3 p0 = Vector3(wp0.x, wp0.y, wp0.z);
                        Vector3 p1 = Vector3(wp1.x, wp1.y, wp1.z);
                        Vector3 p2 = Vector3(wp2.x, wp2.y, wp2.z);
                        sceneMin.x = std::min(sceneMin.x, std::min(p0.x, std::min(p1.x, p2.x)));
                        sceneMin.y = std::min(sceneMin.y, std::min(p0.y, std::min(p1.y, p2.y)));
                        sceneMin.z = std::min(sceneMin.z, std::min(p0.z, std::min(p1.z, p2.z)));
                        sceneMax.x = std::max(sceneMax.x, std::max(p0.x, std::max(p1.x, p2.x)));
                        sceneMax.y = std::max(sceneMax.y, std::max(p0.y, std::max(p1.y, p2.y)));
                        sceneMax.z = std::max(sceneMax.z, std::max(p0.z, std::max(p1.z, p2.z)));

                        float area = 0.5f * length(cross(p1 - p0, p2 - p0));
                        if(!std::isfinite(area) || area <= 1e-10f)
                        {
                            continue;
                        }

                        totalArea += static_cast<double>(area);
                        triangleRefs.push_back({ drawId, tri });
                        triangleCdf.push_back(static_cast<float>(totalArea));
                    }
                }

                if(triangleRefs.empty() || totalArea <= 0.0)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: live shared batch generation found no valid triangle areas.");
                    return false;
                }

                float invTotalArea = 1.0f / static_cast<float>(totalArea);
                for(float& c : triangleCdf)
                {
                    c *= invTotalArea;
                }

                CachedSceneMin = sceneMin;
                CachedSceneMax = sceneMax;
                HasCachedSceneBounds = true;

                uint refsBytes = static_cast<uint>(triangleRefs.size() * sizeof(TrainingTriangleRef));
                if(!TriangleRefsBuffer || TriangleRefsCapacityBytes != refsBytes)
                {
                    if(TriangleRefsBuffer)
                    {
                        Renderer::Destroy(TriangleRefsBuffer);
                        delete TriangleRefsBuffer;
                        TriangleRefsBuffer = nullptr;
                    }

                    TriangleRefsBuffer = Renderer::CreateBuffer("TrainingPathTracingTriangleRefsBuffer", BufferType::StorageBuffer, refsBytes, sizeof(TrainingTriangleRef));
                    TriangleRefsCapacityBytes = refsBytes;
                }
                Renderer::UploadBuffer(TriangleRefsBuffer, triangleRefs.data(), refsBytes);

                uint cdfBytes = static_cast<uint>(triangleCdf.size() * sizeof(float));
                if(!TriangleCdfBuffer || TriangleCdfCapacityBytes != cdfBytes)
                {
                    if(TriangleCdfBuffer)
                    {
                        Renderer::Destroy(TriangleCdfBuffer);
                        delete TriangleCdfBuffer;
                        TriangleCdfBuffer = nullptr;
                    }
                    TriangleCdfBuffer = Renderer::CreateBuffer("TrainingPathTracingTriangleCdfBuffer", BufferType::StorageBuffer, cdfBytes, sizeof(float));
                    TriangleCdfCapacityBytes = cdfBytes;
                }
                Renderer::UploadBuffer(TriangleCdfBuffer, triangleCdf.data(), cdfBytes);

                SamplingResourcesReady = true;
                CachedDrawCommandsCount = drawCommandsCount;
                CachedVertexBufferSize = renderData.VertexBuffer.Size;
                CachedIndexBufferSize = renderData.IndexBuffer.Size;
                CachedNumTriangleRefs = static_cast<uint>(triangleRefs.size());
            }

            uint requiredBytes = requestedSampleCount * sizeof(TrainingSampleCPU);
            if(!TrainingOutputBuffer || TrainingOutputCapacityBytes != requiredBytes)
            {
                if(TrainingOutputBuffer)
                {
                    Renderer::Destroy(TrainingOutputBuffer);
                    delete TrainingOutputBuffer;
                    TrainingOutputBuffer = nullptr;
                }

                TrainingOutputBuffer = Renderer::CreateBuffer("TrainingPathTracingOutputBuffer", BufferType::StorageBuffer, requiredBytes, sizeof(TrainingSampleCPU));
                TrainingOutputCapacityBytes = requiredBytes;
            }

            RootConstants.OutputBufferID = TrainingOutputBuffer->GetIndex(UAV);
            RootConstants.TLASID = renderData.TLAS.GetIndex(SRV_CBV);
            RootConstants.VertexBufferId = renderData.VertexBuffer.GetIndex(SRV_CBV);
            RootConstants.IndexBufferId = renderData.IndexBuffer.GetIndex(SRV_CBV);
            RootConstants.TriangleRefsBufferId = TriangleRefsBuffer->GetIndex(SRV_CBV);
            RootConstants.TriangleCdfBufferId = TriangleCdfBuffer->GetIndex(SRV_CBV);
            RootConstants.DrawCommandsBufferId = renderData.SharedDrawCommandsBuffer->GetIndex(SRV_CBV);
            RootConstants.MaterialBufferId = renderData.SharedMaterialAttributesBuffer->GetIndex(SRV_CBV);
            RootConstants.WorldTransformsBufferId = renderData.SharedWorldTransformsBuffer->GetIndex(SRV_CBV);
            RootConstants.LightsBufferId = renderData.SharedLightsBuffer->GetIndex(SRV_CBV);
            RootConstants.LightTransformsBufferId = renderData.SharedLightTransformsBuffer->GetIndex(SRV_CBV);
            RootConstants.LightsIndicesBufferId = renderData.SharedLightsIndicesBuffer->GetIndex(SRV_CBV);
            RootConstants.NumTriangleRefs = CachedNumTriangleRefs;
            RootConstants.NumLights = renderData.SharedNumLights;
            RootConstants.RaysPerPoint = renderData.TrainingDatasetRaysPerPoint > 0 ? renderData.TrainingDatasetRaysPerPoint : 1;
            uint requestedBounces = renderData.TrainingDatasetMaxBounces > 0 ? renderData.TrainingDatasetMaxBounces : 1;
            RootConstants.MaxBounces = requestedBounces > 4 ? 4 : requestedBounces;
            RootConstants.SurfaceSampleProbability = SurfaceSampleProbability;
            RootConstants.EnableBackfaceCulling = 1u;
            RootConstants.SceneMinX = CachedSceneMin.x;
            RootConstants.SceneMinY = CachedSceneMin.y;
            RootConstants.SceneMinZ = CachedSceneMin.z;
            RootConstants.SceneMinW = 0.0f;
            RootConstants.SceneMaxX = CachedSceneMax.x;
            RootConstants.SceneMaxY = CachedSceneMax.y;
            RootConstants.SceneMaxZ = CachedSceneMax.z;
            RootConstants.SceneMaxW = 0.0f;
            RootConstants.SeedOffset = renderData.TrainingDatasetSeed++;

            Renderer::ResourceBarrier(TrainingOutputBuffer, UNORDERED_ACCESS);
            Renderer::SetPipeline(TrainingPipeline);
            Renderer::PushConstants(&RootConstants, sizeof(TrainingPathTracingRootConstants));
            Renderer::TraceRays(TrainingPipeline, Point3(requestedSampleCount, 1, 1));
            Renderer::UAVBarrier(TrainingOutputBuffer);
            Renderer::ResourceBarrier(TrainingOutputBuffer, ALL_SHADER_RESOURCE);
            Renderer::Flush();

            sharedHandle = Renderer::GetSharedHandle(TrainingOutputBuffer);
            sizeBytes = requiredBytes;
            return sharedHandle != nullptr;
        }

        bool GenerateLiveNIVBatch(Coach::TinyCuda::NIVSample* outSamples, uint requestedSampleCount)
        {
            if(!outSamples || requestedSampleCount == 0)
            {
                return false;
            }

            EnsureTrainingPipeline();
            if(!TrainingPipeline)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation failed because TrainingPipeline is not initialized.");
                return false;
            }

            auto& renderData = Renderer::RenderData;
            if(!renderData.SharedDrawCommandsBuffer || !renderData.SharedMaterialAttributesBuffer || !renderData.SharedWorldTransformsBuffer || renderData.SharedDrawCommandsCount == 0)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation failed because scene buffers are not ready.");
                return false;
            }

            if(renderData.SharedNumLights == 0u)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation failed because scene has no lights.");
                return false;
            }

            if(renderData.SharedNumLights > 0 &&
               (!renderData.SharedLightsBuffer || !renderData.SharedLightTransformsBuffer || !renderData.SharedLightsIndicesBuffer))
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation failed because light buffers are not ready.");
                return false;
            }

            const uint drawCommandsCount = renderData.SharedDrawCommandsCount;
            const bool mustRebuildSamplingResources =
                !SamplingResourcesReady ||
                CachedDrawCommandsCount != drawCommandsCount ||
                CachedVertexBufferSize != renderData.VertexBuffer.Size ||
                CachedIndexBufferSize != renderData.IndexBuffer.Size;

            if(mustRebuildSamplingResources)
            {
                std::vector<DrawIndexedCommand> drawCommandsCPU(drawCommandsCount);
                Renderer::DownloadBuffer(renderData.SharedDrawCommandsBuffer, drawCommandsCPU.data(), drawCommandsCount * sizeof(DrawIndexedCommand));

                std::vector<Matrix4> worldTransformsCPU(drawCommandsCount);
                Renderer::DownloadBuffer(renderData.SharedWorldTransformsBuffer, worldTransformsCPU.data(), drawCommandsCount * sizeof(Matrix4));

                uint vertexCount = renderData.VertexBuffer.Size / sizeof(Vertex);
                uint indexCount = renderData.IndexBuffer.Size / sizeof(uint);
                if(vertexCount == 0 || indexCount == 0)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation failed because vertex/index buffers are empty.");
                    return false;
                }

                std::vector<Vertex> verticesCPU(vertexCount);
                std::vector<uint> indicesCPU(indexCount);
                Renderer::DownloadBuffer(renderData.VertexBuffer.GetBuffer(), verticesCPU.data(), renderData.VertexBuffer.Size);
                Renderer::DownloadBuffer(renderData.IndexBuffer.GetBuffer(), indicesCPU.data(), renderData.IndexBuffer.Size);

                std::vector<TrainingTriangleRef> triangleRefs;
                std::vector<float> triangleCdf;
                triangleRefs.reserve(65536);
                triangleCdf.reserve(65536);
                Vector3 sceneMin(FLT_MAX, FLT_MAX, FLT_MAX);
                Vector3 sceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

                double totalArea = 0.0;
                for(uint drawId = 0; drawId < drawCommandsCount; drawId++)
                {
                    const auto& command = drawCommandsCPU[drawId];
                    if(command.IndexCountPerInstance < 3)
                    {
                        continue;
                    }

                    uint triCount = command.IndexCountPerInstance / 3;
                    for(uint tri = 0; tri < triCount; tri++)
                    {
                        uint i0Idx = command.StartIndexLocation + tri * 3 + 0;
                        uint i1Idx = command.StartIndexLocation + tri * 3 + 1;
                        uint i2Idx = command.StartIndexLocation + tri * 3 + 2;
                        if(i2Idx >= indexCount)
                        {
                            continue;
                        }

                        int baseVertex = command.BaseVertexLocation;
                        int vi0 = static_cast<int>(indicesCPU[i0Idx]) + baseVertex;
                        int vi1 = static_cast<int>(indicesCPU[i1Idx]) + baseVertex;
                        int vi2 = static_cast<int>(indicesCPU[i2Idx]) + baseVertex;
                        if(vi0 < 0 || vi1 < 0 || vi2 < 0 || static_cast<uint>(vi0) >= vertexCount || static_cast<uint>(vi1) >= vertexCount || static_cast<uint>(vi2) >= vertexCount)
                        {
                            continue;
                        }

                        const Matrix4& world = worldTransformsCPU[drawId];
                        Vector4 wp0 = world * verticesCPU[vi0].Position;
                        Vector4 wp1 = world * verticesCPU[vi1].Position;
                        Vector4 wp2 = world * verticesCPU[vi2].Position;
                        Vector3 p0 = Vector3(wp0.x, wp0.y, wp0.z);
                        Vector3 p1 = Vector3(wp1.x, wp1.y, wp1.z);
                        Vector3 p2 = Vector3(wp2.x, wp2.y, wp2.z);
                        sceneMin.x = std::min(sceneMin.x, std::min(p0.x, std::min(p1.x, p2.x)));
                        sceneMin.y = std::min(sceneMin.y, std::min(p0.y, std::min(p1.y, p2.y)));
                        sceneMin.z = std::min(sceneMin.z, std::min(p0.z, std::min(p1.z, p2.z)));
                        sceneMax.x = std::max(sceneMax.x, std::max(p0.x, std::max(p1.x, p2.x)));
                        sceneMax.y = std::max(sceneMax.y, std::max(p0.y, std::max(p1.y, p2.y)));
                        sceneMax.z = std::max(sceneMax.z, std::max(p0.z, std::max(p1.z, p2.z)));

                        float area = 0.5f * length(cross(p1 - p0, p2 - p0));
                        if(!std::isfinite(area) || area <= 1e-10f)
                        {
                            continue;
                        }

                        totalArea += static_cast<double>(area);
                        triangleRefs.push_back({ drawId, tri });
                        triangleCdf.push_back(static_cast<float>(totalArea));
                    }
                }

                if(triangleRefs.empty() || totalArea <= 0.0)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation found no valid triangle areas.");
                    return false;
                }

                float invTotalArea = 1.0f / static_cast<float>(totalArea);
                for(float& c : triangleCdf)
                {
                    c *= invTotalArea;
                }

                CachedSceneMin = sceneMin;
                CachedSceneMax = sceneMax;
                HasCachedSceneBounds = true;

                uint refsBytes = static_cast<uint>(triangleRefs.size() * sizeof(TrainingTriangleRef));
                if(!TriangleRefsBuffer || TriangleRefsCapacityBytes != refsBytes)
                {
                    if(TriangleRefsBuffer)
                    {
                        Renderer::Destroy(TriangleRefsBuffer);
                        delete TriangleRefsBuffer;
                        TriangleRefsBuffer = nullptr;
                    }

                    TriangleRefsBuffer = Renderer::CreateBuffer("TrainingPathTracingTriangleRefsBuffer", BufferType::StorageBuffer, refsBytes, sizeof(TrainingTriangleRef));
                    TriangleRefsCapacityBytes = refsBytes;
                }
                Renderer::UploadBuffer(TriangleRefsBuffer, triangleRefs.data(), refsBytes);

                uint cdfBytes = static_cast<uint>(triangleCdf.size() * sizeof(float));
                if(!TriangleCdfBuffer || TriangleCdfCapacityBytes != cdfBytes)
                {
                    if(TriangleCdfBuffer)
                    {
                        Renderer::Destroy(TriangleCdfBuffer);
                        delete TriangleCdfBuffer;
                        TriangleCdfBuffer = nullptr;
                    }
                    TriangleCdfBuffer = Renderer::CreateBuffer("TrainingPathTracingTriangleCdfBuffer", BufferType::StorageBuffer, cdfBytes, sizeof(float));
                    TriangleCdfCapacityBytes = cdfBytes;
                }
                Renderer::UploadBuffer(TriangleCdfBuffer, triangleCdf.data(), cdfBytes);

                SamplingResourcesReady = true;
                CachedDrawCommandsCount = drawCommandsCount;
                CachedVertexBufferSize = renderData.VertexBuffer.Size;
                CachedIndexBufferSize = renderData.IndexBuffer.Size;
                CachedNumTriangleRefs = static_cast<uint>(triangleRefs.size());
            }

            uint requiredBytes = requestedSampleCount * sizeof(TrainingSampleCPU);
            if(!TrainingOutputBuffer || TrainingOutputCapacityBytes != requiredBytes)
            {
                if(TrainingOutputBuffer)
                {
                    Renderer::Destroy(TrainingOutputBuffer);
                    delete TrainingOutputBuffer;
                    TrainingOutputBuffer = nullptr;
                }

                TrainingOutputBuffer = Renderer::CreateBuffer("TrainingPathTracingOutputBuffer", BufferType::StorageBuffer, requiredBytes, sizeof(TrainingSampleCPU));
                TrainingOutputCapacityBytes = requiredBytes;
            }

            RootConstants.OutputBufferID = TrainingOutputBuffer->GetIndex(UAV);
            RootConstants.TLASID = renderData.TLAS.GetIndex(SRV_CBV);
            RootConstants.VertexBufferId = renderData.VertexBuffer.GetIndex(SRV_CBV);
            RootConstants.IndexBufferId = renderData.IndexBuffer.GetIndex(SRV_CBV);
            RootConstants.TriangleRefsBufferId = TriangleRefsBuffer->GetIndex(SRV_CBV);
            RootConstants.TriangleCdfBufferId = TriangleCdfBuffer->GetIndex(SRV_CBV);
            RootConstants.DrawCommandsBufferId = renderData.SharedDrawCommandsBuffer->GetIndex(SRV_CBV);
            RootConstants.MaterialBufferId = renderData.SharedMaterialAttributesBuffer->GetIndex(SRV_CBV);
            RootConstants.WorldTransformsBufferId = renderData.SharedWorldTransformsBuffer->GetIndex(SRV_CBV);
            RootConstants.LightsBufferId = renderData.SharedLightsBuffer->GetIndex(SRV_CBV);
            RootConstants.LightTransformsBufferId = renderData.SharedLightTransformsBuffer->GetIndex(SRV_CBV);
            RootConstants.LightsIndicesBufferId = renderData.SharedLightsIndicesBuffer->GetIndex(SRV_CBV);
            RootConstants.NumTriangleRefs = CachedNumTriangleRefs;
            RootConstants.NumLights = renderData.SharedNumLights;
            RootConstants.RaysPerPoint = renderData.TrainingDatasetRaysPerPoint > 0 ? renderData.TrainingDatasetRaysPerPoint : 1;
            uint requestedBounces = renderData.TrainingDatasetMaxBounces > 0 ? renderData.TrainingDatasetMaxBounces : 1;
            RootConstants.MaxBounces = requestedBounces > 4 ? 4 : requestedBounces;
            RootConstants.SurfaceSampleProbability = SurfaceSampleProbability;
            RootConstants.EnableBackfaceCulling = 1u;
            RootConstants.SceneMinX = CachedSceneMin.x;
            RootConstants.SceneMinY = CachedSceneMin.y;
            RootConstants.SceneMinZ = CachedSceneMin.z;
            RootConstants.SceneMinW = 0.0f;
            RootConstants.SceneMaxX = CachedSceneMax.x;
            RootConstants.SceneMaxY = CachedSceneMax.y;
            RootConstants.SceneMaxZ = CachedSceneMax.z;
            RootConstants.SceneMaxW = 0.0f;

            std::vector<TrainingSampleCPU> samples(requestedSampleCount);
            std::vector<TrainingSampleCPU> acceptedSamples;
            acceptedSamples.reserve(requestedSampleCount * 4);

            uint attempts = 0;
            const uint maxAttempts = 64;
            const uint targetCandidateCount = requestedSampleCount * 4;
            while(acceptedSamples.size() < targetCandidateCount && attempts < maxAttempts)
            {
                RootConstants.SeedOffset = renderData.TrainingDatasetSeed++;

                Renderer::ResourceBarrier(TrainingOutputBuffer, UNORDERED_ACCESS);
                Renderer::SetPipeline(TrainingPipeline);
                Renderer::PushConstants(&RootConstants, sizeof(TrainingPathTracingRootConstants));
                Renderer::TraceRays(TrainingPipeline, Point3(requestedSampleCount, 1, 1));
                Renderer::UAVBarrier(TrainingOutputBuffer);
                Renderer::ResourceBarrier(TrainingOutputBuffer, ALL_SHADER_RESOURCE);
                Renderer::Flush();

                Renderer::DownloadBuffer(TrainingOutputBuffer, samples.data(), requiredBytes);
                for(const auto& s : samples)
                {
                    const float r = s.DiffuseIrradiance.x;
                    const float g = s.DiffuseIrradiance.y;
                    const float b = s.DiffuseIrradiance.z;
                    if(r > 1e-8f || g > 1e-8f || b > 1e-8f)
                    {
                        acceptedSamples.push_back(s);
                        if(acceptedSamples.size() >= targetCandidateCount)
                        {
                            break;
                        }
                    }
                }

                attempts++;
            }

            if(acceptedSamples.size() < requestedSampleCount)
            {
                WD_CORE_ERROR("TrainingPathTracingSystem: live batch generation accepted only {0}/{1} non-null samples after {2} attempts.",
                    static_cast<uint>(acceptedSamples.size()), requestedSampleCount, attempts);
                return false;
            }

            std::sort(acceptedSamples.begin(), acceptedSamples.end(), [](const TrainingSampleCPU& a, const TrainingSampleCPU& b)
            {
                const float lumA =
                    a.DiffuseIrradiance.x * 0.2126f +
                    a.DiffuseIrradiance.y * 0.7152f +
                    a.DiffuseIrradiance.z * 0.0722f;
                const float lumB =
                    b.DiffuseIrradiance.x * 0.2126f +
                    b.DiffuseIrradiance.y * 0.7152f +
                    b.DiffuseIrradiance.z * 0.0722f;
                return lumA > lumB;
            });

            const uint preferredHighEnergyCount = std::min<uint>(requestedSampleCount / 4u, static_cast<uint>(acceptedSamples.size()));
            std::vector<TrainingSampleCPU> selectedSamples;
            selectedSamples.reserve(requestedSampleCount);

            for(uint i = 0; i < preferredHighEnergyCount; ++i)
            {
                selectedSamples.push_back(acceptedSamples[i]);
            }

            static thread_local std::mt19937 rng(std::random_device{}());
            if(acceptedSamples.size() > preferredHighEnergyCount && selectedSamples.size() < requestedSampleCount)
            {
                std::uniform_int_distribution<size_t> remainingDist(preferredHighEnergyCount, acceptedSamples.size() - 1);
                while(selectedSamples.size() < requestedSampleCount)
                {
                    selectedSamples.push_back(acceptedSamples[remainingDist(rng)]);
                }
            }

            for(uint i = 0; i < requestedSampleCount; ++i)
            {
                const auto& s = selectedSamples[i];
                auto& out = outSamples[i];
                out.px = s.WorldPosition.x;
                out.py = s.WorldPosition.y;
                out.pz = s.WorldPosition.z;
                out.nx = s.WorldNormal.x;
                out.ny = s.WorldNormal.y;
                out.nz = s.WorldNormal.z;
                out.ir = s.DiffuseIrradiance.x;
                out.ig = s.DiffuseIrradiance.y;
                out.ib = s.DiffuseIrradiance.z;
            }

            return true;
        }

        void Initialize() override
        {
            EnsureTrainingPipeline();

            ECS::World.system("TrainingPathTracingCaptureSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                if(viewport != ViewportManager::GetEditorViewport())
                {
                    return;
                }

                auto& renderData = Renderer::RenderData;
                if(!renderData.RequestTrainingDatasetCapture)
                {
                    return;
                }

                if(!renderData.SharedDrawCommandsBuffer || !renderData.SharedMaterialAttributesBuffer || !renderData.SharedWorldTransformsBuffer || renderData.SharedDrawCommandsCount == 0)
                {
                    renderData.RequestTrainingDatasetCapture = true;
                    WD_CORE_WARN("TrainingPathTracingSystem: scene buffers not ready yet, capture will retry next frame.");
                    return;
                }

                if(renderData.SharedNumLights > 0 &&
                   (!renderData.SharedLightsBuffer || !renderData.SharedLightTransformsBuffer || !renderData.SharedLightsIndicesBuffer))
                {
                    renderData.RequestTrainingDatasetCapture = true;
                    WD_CORE_WARN("TrainingPathTracingSystem: light buffers not ready yet (NumLights={0}), capture will retry next frame.", renderData.SharedNumLights);
                    return;
                }
                renderData.RequestTrainingDatasetCapture = false;

                const uint drawCommandsCount = renderData.SharedDrawCommandsCount;

                std::vector<DrawIndexedCommand> drawCommandsCPU(drawCommandsCount);
                Renderer::DownloadBuffer(renderData.SharedDrawCommandsBuffer, drawCommandsCPU.data(), drawCommandsCount * sizeof(DrawIndexedCommand));

                std::vector<Matrix4> worldTransformsCPU(drawCommandsCount);
                Renderer::DownloadBuffer(renderData.SharedWorldTransformsBuffer, worldTransformsCPU.data(), drawCommandsCount * sizeof(Matrix4));

                uint vertexCount = renderData.VertexBuffer.Size / sizeof(Vertex);
                uint indexCount = renderData.IndexBuffer.Size / sizeof(uint);
                if(vertexCount == 0 || indexCount == 0)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: vertex/index buffers are empty.");
                    return;
                }

                std::vector<Vertex> verticesCPU(vertexCount);
                std::vector<uint> indicesCPU(indexCount);
                Renderer::DownloadBuffer(renderData.VertexBuffer.GetBuffer(), verticesCPU.data(), renderData.VertexBuffer.Size);
                Renderer::DownloadBuffer(renderData.IndexBuffer.GetBuffer(), indicesCPU.data(), renderData.IndexBuffer.Size);

                std::vector<TrainingTriangleRef> triangleRefs;
                std::vector<float> triangleCdf;
                triangleRefs.reserve(65536);
                triangleCdf.reserve(65536);
                Vector3 sceneMin(FLT_MAX, FLT_MAX, FLT_MAX);
                Vector3 sceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

                double totalArea = 0.0;
                for(uint drawId = 0; drawId < drawCommandsCount; drawId++)
                {
                    const auto& command = drawCommandsCPU[drawId];
                    if(command.IndexCountPerInstance < 3)
                    {
                        continue;
                    }

                    uint triCount = command.IndexCountPerInstance / 3;
                    for(uint tri = 0; tri < triCount; tri++)
                    {
                        uint i0Idx = command.StartIndexLocation + tri * 3 + 0;
                        uint i1Idx = command.StartIndexLocation + tri * 3 + 1;
                        uint i2Idx = command.StartIndexLocation + tri * 3 + 2;
                        if(i2Idx >= indexCount)
                        {
                            continue;
                        }

                        int baseVertex = command.BaseVertexLocation;
                        int vi0 = static_cast<int>(indicesCPU[i0Idx]) + baseVertex;
                        int vi1 = static_cast<int>(indicesCPU[i1Idx]) + baseVertex;
                        int vi2 = static_cast<int>(indicesCPU[i2Idx]) + baseVertex;
                        if(vi0 < 0 || vi1 < 0 || vi2 < 0 || static_cast<uint>(vi0) >= vertexCount || static_cast<uint>(vi1) >= vertexCount || static_cast<uint>(vi2) >= vertexCount)
                        {
                            continue;
                        }

                        const Matrix4& world = worldTransformsCPU[drawId];
                        Vector4 wp0 = world * verticesCPU[vi0].Position;
                        Vector4 wp1 = world * verticesCPU[vi1].Position;
                        Vector4 wp2 = world * verticesCPU[vi2].Position;
                        Vector3 p0 = Vector3(wp0.x, wp0.y, wp0.z);
                        Vector3 p1 = Vector3(wp1.x, wp1.y, wp1.z);
                        Vector3 p2 = Vector3(wp2.x, wp2.y, wp2.z);
                        sceneMin.x = std::min(sceneMin.x, std::min(p0.x, std::min(p1.x, p2.x)));
                        sceneMin.y = std::min(sceneMin.y, std::min(p0.y, std::min(p1.y, p2.y)));
                        sceneMin.z = std::min(sceneMin.z, std::min(p0.z, std::min(p1.z, p2.z)));
                        sceneMax.x = std::max(sceneMax.x, std::max(p0.x, std::max(p1.x, p2.x)));
                        sceneMax.y = std::max(sceneMax.y, std::max(p0.y, std::max(p1.y, p2.y)));
                        sceneMax.z = std::max(sceneMax.z, std::max(p0.z, std::max(p1.z, p2.z)));

                        float area = 0.5f * length(cross(p1 - p0, p2 - p0));
                        if(!std::isfinite(area) || area <= 1e-10f)
                        {
                            continue;
                        }

                        totalArea += static_cast<double>(area);
                        triangleRefs.push_back({ drawId, tri });
                        triangleCdf.push_back(static_cast<float>(totalArea));
                    }
                }

                if(triangleRefs.empty() || totalArea <= 0.0)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: no valid triangle areas found for sampling.");
                    return;
                }

                float invTotalArea = 1.0f / static_cast<float>(totalArea);
                for(float& c : triangleCdf)
                {
                    c *= invTotalArea;
                }

                uint refsBytes = static_cast<uint>(triangleRefs.size() * sizeof(TrainingTriangleRef));
                if(!TriangleRefsBuffer || TriangleRefsCapacityBytes != refsBytes)
                {
                    if(TriangleRefsBuffer)
                    {
                        Renderer::Destroy(TriangleRefsBuffer);
                        delete TriangleRefsBuffer;
                        TriangleRefsBuffer = nullptr;
                    }
                    
                    TriangleRefsBuffer = Renderer::CreateBuffer("TrainingPathTracingTriangleRefsBuffer", BufferType::StorageBuffer, refsBytes, sizeof(TrainingTriangleRef));
                    TriangleRefsCapacityBytes = refsBytes;
                }
                Renderer::UploadBuffer(TriangleRefsBuffer, triangleRefs.data(), refsBytes);

                uint cdfBytes = static_cast<uint>(triangleCdf.size() * sizeof(float));
                if(!TriangleCdfBuffer || TriangleCdfCapacityBytes != cdfBytes)
                {
                    if(TriangleCdfBuffer)
                    {
                        Renderer::Destroy(TriangleCdfBuffer);
                        delete TriangleCdfBuffer;
                        TriangleCdfBuffer = nullptr;
                    }
                    TriangleCdfBuffer = Renderer::CreateBuffer("TrainingPathTracingTriangleCdfBuffer", BufferType::StorageBuffer, cdfBytes, sizeof(float));
                    TriangleCdfCapacityBytes = cdfBytes;
                }
                Renderer::UploadBuffer(TriangleCdfBuffer, triangleCdf.data(), cdfBytes);

                const uint requestedSampleCount = renderData.TrainingDatasetSampleCount > 0 ? renderData.TrainingDatasetSampleCount : 1;
                const uint batchCount = renderData.TrainingDatasetCaptureBatches > 0 ? renderData.TrainingDatasetCaptureBatches : 1;
                uint sampleCount = requestedSampleCount;
                uint requiredBytes = sampleCount * sizeof(TrainingSampleCPU);
                if(!TrainingOutputBuffer || TrainingOutputCapacityBytes != requiredBytes)
                {
                    if(TrainingOutputBuffer)
                    {
                        Renderer::Destroy(TrainingOutputBuffer);
                        delete TrainingOutputBuffer;
                        TrainingOutputBuffer = nullptr;
                    }

                    TrainingOutputBuffer = Renderer::CreateBuffer("TrainingPathTracingOutputBuffer", BufferType::StorageBuffer, requiredBytes, sizeof(TrainingSampleCPU));
                    TrainingOutputCapacityBytes = requiredBytes;
                }

                RootConstants.OutputBufferID = TrainingOutputBuffer->GetIndex(UAV);
                RootConstants.TLASID = renderData.TLAS.GetIndex(SRV_CBV);
                RootConstants.VertexBufferId = renderData.VertexBuffer.GetIndex(SRV_CBV);
                RootConstants.IndexBufferId = renderData.IndexBuffer.GetIndex(SRV_CBV);
                RootConstants.TriangleRefsBufferId = TriangleRefsBuffer->GetIndex(SRV_CBV);
                RootConstants.TriangleCdfBufferId = TriangleCdfBuffer->GetIndex(SRV_CBV);
                RootConstants.DrawCommandsBufferId = renderData.SharedDrawCommandsBuffer->GetIndex(SRV_CBV);
                RootConstants.MaterialBufferId = renderData.SharedMaterialAttributesBuffer->GetIndex(SRV_CBV);
                RootConstants.WorldTransformsBufferId = renderData.SharedWorldTransformsBuffer->GetIndex(SRV_CBV);
                RootConstants.LightsBufferId = renderData.SharedLightsBuffer ? renderData.SharedLightsBuffer->GetIndex(SRV_CBV) : 0;
                RootConstants.LightTransformsBufferId = renderData.SharedLightTransformsBuffer ? renderData.SharedLightTransformsBuffer->GetIndex(SRV_CBV) : 0;
                RootConstants.LightsIndicesBufferId = renderData.SharedLightsIndicesBuffer ? renderData.SharedLightsIndicesBuffer->GetIndex(SRV_CBV) : 0;
                RootConstants.NumTriangleRefs = static_cast<uint>(triangleRefs.size());
                RootConstants.NumLights = renderData.SharedNumLights;
                if(RootConstants.NumLights == 0u)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: NumLights is 0 during capture. Aborting dataset write to avoid all-zero targets.");
                    return;
                }
                RootConstants.RaysPerPoint = renderData.TrainingDatasetRaysPerPoint > 0 ? renderData.TrainingDatasetRaysPerPoint : 1;
                uint requestedBounces = renderData.TrainingDatasetMaxBounces > 0 ? renderData.TrainingDatasetMaxBounces : 1;
                RootConstants.MaxBounces = requestedBounces > 4 ? 4 : requestedBounces;
                RootConstants.SurfaceSampleProbability = SurfaceSampleProbability;
                RootConstants.EnableBackfaceCulling = 1u;
                RootConstants.SceneMinX = sceneMin.x;
                RootConstants.SceneMinY = sceneMin.y;
                RootConstants.SceneMinZ = sceneMin.z;
                RootConstants.SceneMinW = 0.0f;
                RootConstants.SceneMaxX = sceneMax.x;
                RootConstants.SceneMaxY = sceneMax.y;
                RootConstants.SceneMaxZ = sceneMax.z;
                RootConstants.SceneMaxW = 0.0f;

                Path outputPath = ResolveOutputPath(renderData.TrainingDatasetOutputPath);
                std::filesystem::create_directories(outputPath.parent_path());

                std::ofstream out(outputPath.c_str(), std::ios::binary | std::ios::trunc);
                if(!out)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: failed to open output file: {0}", outputPath.string().c_str());
                    return;
                }

                uint64 totalSamples64 = static_cast<uint64>(requestedSampleCount) * static_cast<uint64>(batchCount);
                if(totalSamples64 > 0xFFFFFFFFull)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: sampleCount * batches exceeds uint32 header capacity.");
                    return;
                }

                TrainingDatasetHeader header;
                header.SampleCount = static_cast<uint32>(totalSamples64);
                header.SampleStride = sizeof(TrainingSampleCPU);
                out.write(reinterpret_cast<const char*>(&header), sizeof(header));

                std::vector<TrainingSampleCPU> samples(sampleCount);
                std::vector<TrainingSampleCPU> acceptedSamples;
                acceptedSamples.reserve(requestedSampleCount);
                for(uint batchIndex = 0; batchIndex < batchCount; batchIndex++)
                {
                    acceptedSamples.clear();
                    uint attempts = 0;
                    const uint maxAttempts = 64;

                    while(acceptedSamples.size() < requestedSampleCount && attempts < maxAttempts)
                    {
                        RootConstants.SeedOffset = renderData.TrainingDatasetSeed++;

                        Renderer::ResourceBarrier(TrainingOutputBuffer, UNORDERED_ACCESS);
                        Renderer::SetPipeline(TrainingPipeline);
                        Renderer::PushConstants(&RootConstants, sizeof(TrainingPathTracingRootConstants));
                        Renderer::TraceRays(TrainingPipeline, Point3(sampleCount, 1, 1));
                        Renderer::UAVBarrier(TrainingOutputBuffer);
                        Renderer::ResourceBarrier(TrainingOutputBuffer, ALL_SHADER_RESOURCE);
                        Renderer::Wait();

                        Renderer::DownloadBuffer(TrainingOutputBuffer, samples.data(), requiredBytes);

                        for(const auto& s : samples)
                        {
                            const float r = s.DiffuseIrradiance.x;
                            const float g = s.DiffuseIrradiance.y;
                            const float b = s.DiffuseIrradiance.z;
                            if(r > 1e-8f || g > 1e-8f || b > 1e-8f)
                            {
                                acceptedSamples.push_back(s);
                                if(acceptedSamples.size() >= requestedSampleCount)
                                {
                                    break;
                                }
                            }
                        }

                        attempts++;
                    }

                    if(acceptedSamples.size() < requestedSampleCount)
                    {
                        WD_CORE_ERROR("TrainingPathTracingSystem: failed to gather enough non-null irradiance samples for batch {0}. Accepted {1}/{2} after {3} attempts.",
                            batchIndex,
                            static_cast<uint>(acceptedSamples.size()),
                            requestedSampleCount,
                            attempts);
                        out.close();
                        std::error_code ec;
                        std::filesystem::remove(outputPath, ec);
                        return;
                    }

                    out.write(reinterpret_cast<const char*>(acceptedSamples.data()), requestedSampleCount * sizeof(TrainingSampleCPU));

                    if(batchIndex == 0)
                    {
                        double sumR = 0.0;
                        double sumG = 0.0;
                        double sumB = 0.0;
                        double maxR = 0.0;
                        double maxG = 0.0;
                        double maxB = 0.0;
                        for(const auto& s : acceptedSamples)
                        {
                            const float r = s.DiffuseIrradiance.x;
                            const float g = s.DiffuseIrradiance.y;
                            const float b = s.DiffuseIrradiance.z;
                            sumR += r;
                            sumG += g;
                            sumB += b;
                            maxR = std::max(maxR, static_cast<double>(r));
                            maxG = std::max(maxG, static_cast<double>(g));
                            maxB = std::max(maxB, static_cast<double>(b));
                        }
                        const double inv = acceptedSamples.empty() ? 0.0 : (1.0 / static_cast<double>(acceptedSamples.size()));
                        WD_CORE_INFO("TrainingPathTracingSystem: batch0 accepted irradiance stats meanRGB=({0}, {1}, {2}) maxRGB=({3}, {4}, {5}) accepted={6}/{7} attempts={8}",
                            sumR * inv, sumG * inv, sumB * inv, maxR, maxG, maxB,
                            static_cast<uint>(acceptedSamples.size()), requestedSampleCount, attempts);
                    }

                    if(renderData.TrainingDatasetDebugOutput && batchIndex == 0)
                    {
                        SaveDebugTextures(acceptedSamples, outputPath);
                    }
                }
                out.close();

                WD_CORE_INFO("TrainingPathTracingSystem: wrote {0} non-null samples ({1} batches x {2}) to {3}", static_cast<uint>(totalSamples64), batchCount, requestedSampleCount, outputPath.string().c_str());
            });
        }
    };

    class WALDEM_API LiveNIVTrainingDataProvider : public Coach::TinyCuda::INIVTrainingDataProvider
    {
    public:
        void GenerateBatch(Coach::TinyCuda::NIVSample* out, uint32 count) override
        {
            auto* system = TrainingPathTracingSystem::Get();
            if(!system || !system->GenerateLiveNIVBatch(out, count))
            {
                throw std::runtime_error("[NIV] Live training provider failed to generate a training batch.");
            }
        }

        bool GetPositionBounds(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ) const
        {
            auto* system = TrainingPathTracingSystem::Get();
            return system ? system->GetSceneBounds(minX, minY, minZ, maxX, maxY, maxZ) : false;
        }

        bool SupportsSharedBatches() const override
        {
            return false;
        }

        bool GenerateBatchShared(uint32 count, void*& sharedHandle, size_t& sizeBytes, uint32& sampleStride) override
        {
            auto* system = TrainingPathTracingSystem::Get();
            return system ? system->GenerateLiveNIVBatchShared(count, sharedHandle, sizeBytes, sampleStride) : false;
        }
    };
}
