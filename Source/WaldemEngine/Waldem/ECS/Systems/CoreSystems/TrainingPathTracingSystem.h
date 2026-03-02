#pragma once
#include "Waldem/ECS/Systems/CoreSystem.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Types/BasicTypes.h"
#include "Waldem/Types/String.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cfloat>
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
    };

    struct TrainingDatasetHeader
    {
        uint32 Magic = 0x44505454; // "TTPD"
        uint32 Version = 1;
        uint32 SampleCount = 0;
        uint32 SampleStride = 0;
    };

    struct TrainingSampleCPU
    {
        Vector4 WorldPosition;
        Vector4 WorldNormal;
        Vector4 DiffuseIrradiance;
    };

    struct TrainingTriangleRef
    {
        uint DrawId;
        uint TriangleId;
    };

    class WALDEM_API TrainingPathTracingSystem : public ICoreSystem
    {
        RayTracingShader* TrainingShader = nullptr;
        Pipeline* TrainingPipeline = nullptr;
        Buffer* TrainingOutputBuffer = nullptr;
        Buffer* TriangleRefsBuffer = nullptr;
        Buffer* TriangleCdfBuffer = nullptr;
        uint TriangleRefsCapacityBytes = 0;
        uint TriangleCdfCapacityBytes = 0;
        uint TrainingOutputCapacityBytes = 0;
        TrainingPathTracingRootConstants RootConstants = {};

        static Path ResolveOutputPath(const WString& configuredPath)
        {
            Path outputPath = configuredPath.ToString();
            if(outputPath.is_absolute())
            {
                return outputPath;
            }

            Path contentRoot = Path(CONTENT_PATH);
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
        TrainingPathTracingSystem() {}

        void Initialize() override
        {
            TrainingShader = Renderer::LoadRayTracingShader("RayTracing/TrainingPathTracing");
            TrainingPipeline = Renderer::CreateRayTracingPipeline("TrainingPathTracingPipeline", TrainingShader);

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

                uint sampleCount = renderData.TrainingDatasetSampleCount > 0 ? renderData.TrainingDatasetSampleCount : 1;
                uint batchCount = renderData.TrainingDatasetCaptureBatches > 0 ? renderData.TrainingDatasetCaptureBatches : 1;
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
                RootConstants.RaysPerPoint = renderData.TrainingDatasetRaysPerPoint > 0 ? renderData.TrainingDatasetRaysPerPoint : 1;
                uint requestedBounces = renderData.TrainingDatasetMaxBounces > 0 ? renderData.TrainingDatasetMaxBounces : 1;
                RootConstants.MaxBounces = requestedBounces > 4 ? 4 : requestedBounces;

                Path outputPath = ResolveOutputPath(renderData.TrainingDatasetOutputPath);
                std::filesystem::create_directories(outputPath.parent_path());

                std::ofstream out(outputPath.c_str(), std::ios::binary | std::ios::trunc);
                if(!out)
                {
                    WD_CORE_ERROR("TrainingPathTracingSystem: failed to open output file: {0}", outputPath.string().c_str());
                    return;
                }

                uint64 totalSamples64 = static_cast<uint64>(sampleCount) * static_cast<uint64>(batchCount);
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
                for(uint batchIndex = 0; batchIndex < batchCount; batchIndex++)
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
                    out.write(reinterpret_cast<const char*>(samples.data()), requiredBytes);

                    if(renderData.TrainingDatasetDebugOutput && batchIndex == 0)
                    {
                        SaveDebugTextures(samples, outputPath);
                    }
                }
                out.close();

                WD_CORE_INFO("TrainingPathTracingSystem: wrote {0} samples ({1} batches x {2}) to {3}", static_cast<uint>(totalSamples64), batchCount, sampleCount, outputPath.string().c_str());
            });
        }
    };
}
