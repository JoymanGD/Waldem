#include "NIVCoach.h"
// Force custom-build refresh when interface changes.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <random>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <cfloat>
#include <excpt.h>

#include "json/json.hpp"
#include <cuda_runtime.h>
#include <cuda.h>
#include <tiny-cuda-nn/config.h>
#include <tiny-cuda-nn/gpu_matrix.h>

#ifndef CONTENT_PATH
#define CONTENT_PATH "Content"
#endif

// Must be a plain C function (no C++ objects, no C++ EH) so __try/__except is valid.
static int SafeCudaGetDeviceCount() noexcept {
    int count = 0;
    __try {
        cudaGetDeviceCount(&count);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
    return count;
}

namespace Waldem
{
    namespace Coach
    {
        namespace TinyCuda
        {
            namespace
            {
                struct TrainingDatasetHeader
                {
                    uint32_t Magic = 0x44505454; // "TTPD"
                    uint32_t Version = 1;
                    uint32_t SampleCount = 0;
                    uint32_t SampleStride = 0;
                };

                struct TrainingSampleCPU
                {
                    float WorldPosition[4];
                    float WorldNormal[4];
                    float DiffuseIrradiance[4];
                };

                static std::filesystem::path ResolveProjectContentRoot()
                {
                    std::error_code ec;
                    std::filesystem::path current = std::filesystem::current_path(ec);
                    if (ec)
                    {
                        current.clear();
                    }

                    auto isProjectRoot = [](const std::filesystem::path& candidate) -> bool
                    {
                        std::error_code localEc;
                        return std::filesystem::exists(candidate / "Content", localEc) &&
                               std::filesystem::exists(candidate / "Source", localEc);
                    };

                    for (std::filesystem::path probe = current; !probe.empty(); probe = probe.parent_path())
                    {
                        if (isProjectRoot(probe))
                        {
                            return (probe / "Content").lexically_normal();
                        }

                        if (probe == probe.root_path())
                        {
                            break;
                        }
                    }

                    return std::filesystem::absolute(std::filesystem::path(CONTENT_PATH)).lexically_normal();
                }

                static std::filesystem::path ResolveDatasetPath(const std::filesystem::path& preferredPath)
                {
                    const std::filesystem::path contentRoot = ResolveProjectContentRoot();
                    auto resolvePreferred = [&](const std::filesystem::path& path) -> std::filesystem::path
                    {
                        if (path.empty())
                        {
                            return {};
                        }

                        if (path.is_absolute())
                        {
                            return path;
                        }

                        const std::string generic = path.generic_string();
                        if (generic.rfind("Content/", 0) == 0 || generic == "Content")
                        {
                            return contentRoot.parent_path() / path;
                        }

                        return {};
                    };

                    const std::vector<std::filesystem::path> candidates =
                    {
                        resolvePreferred(preferredPath),
                        std::filesystem::path("Content") / "Training" / "irradiance_samples.bin",
                        std::filesystem::path("..") / "Content" / "Training" / "irradiance_samples.bin",
                        std::filesystem::path("..") / ".." / "Content" / "Training" / "irradiance_samples.bin",
                        std::filesystem::path("..") / ".." / ".." / "Content" / "Training" / "irradiance_samples.bin",
                        contentRoot / "Training" / "irradiance_samples.bin"
                    };

                    for (const auto& candidate : candidates)
                    {
                        const auto absolute = std::filesystem::absolute(candidate);
                        if (std::filesystem::exists(absolute))
                        {
                            return absolute;
                        }
                    }

                    return {};
                }

                static std::filesystem::path ResolveCheckpointReadPath(const std::filesystem::path& checkpointPath)
                {
                    const std::filesystem::path contentRoot = ResolveProjectContentRoot();
                    auto resolvePreferred = [&](const std::filesystem::path& path) -> std::filesystem::path
                    {
                        if (path.empty())
                        {
                            return {};
                        }

                        if (path.is_absolute())
                        {
                            return path;
                        }

                        const std::string generic = path.generic_string();
                        if (generic.rfind("Content/", 0) == 0 || generic == "Content")
                        {
                            return contentRoot.parent_path() / path;
                        }

                        return {};
                    };

                    const std::vector<std::filesystem::path> candidates =
                    {
                        resolvePreferred(checkpointPath),
                        std::filesystem::path("Content") / "Training" / checkpointPath,
                        std::filesystem::path("..") / "Content" / "Training" / checkpointPath,
                        std::filesystem::path("..") / ".." / "Content" / "Training" / checkpointPath,
                        std::filesystem::path("..") / ".." / ".." / "Content" / "Training" / checkpointPath,
                        contentRoot / "Training" / checkpointPath.filename()
                    };

                    for (const auto& candidate : candidates)
                    {
                        const auto absolute = std::filesystem::absolute(candidate);
                        if (std::filesystem::exists(absolute))
                        {
                            return absolute;
                        }
                    }

                    return {};
                }

                static std::filesystem::path ResolveCheckpointWritePath(const std::filesystem::path& checkpointPath)
                {
                    if (checkpointPath.empty())
                    {
                        return {};
                    }

                    const std::filesystem::path contentRoot = ResolveProjectContentRoot();
                    if (checkpointPath.is_absolute())
                    {
                        return checkpointPath.lexically_normal();
                    }

                    const std::string generic = checkpointPath.generic_string();
                    if (generic.rfind("Content/", 0) == 0 || generic == "Content")
                    {
                        return (contentRoot.parent_path() / checkpointPath).lexically_normal();
                    }

                    return (contentRoot / "Training" / checkpointPath.filename()).lexically_normal();
                }

                inline float ClampNonNegative(float value)
                {
                    return std::max(value, 0.0f);
                }

                inline float NormalizeCoord(float value, float minValue, float maxValue)
                {
                    const float extent = maxValue - minValue;
                    if (extent <= 1e-12f)
                    {
                        return 0.0f;
                    }
                    const float t = (value - minValue) / extent;
                    return t * 2.0f - 1.0f;
                }

                struct DeviceInferenceStats
                {
                    uint32_t ValidCount;
                    float LuminanceSum;
                    uint32_t MaxChannelBits;
                };

                struct InteropBuffer
                {
                    void* SharedHandle = nullptr;
                    size_t SizeBytes = 0;
                    cudaExternalMemory_t ExternalMemory = nullptr;
                    void* DevicePtr = nullptr;
                };

                __device__ inline float ClampNonNegativeDevice(float value)
                {
                    return value > 0.0f ? value : 0.0f;
                }

                __device__ inline void OctEncodeDevice(float x, float y, float z, float& outU, float& outV)
                {
                    const float invL1Norm = 1.0f / (fabsf(x) + fabsf(y) + fabsf(z) + 1e-8f);
                    x *= invL1Norm;
                    y *= invL1Norm;
                    z *= invL1Norm;

                    if (z < 0.0f)
                    {
                        const float oldX = x;
                        x = (1.0f - fabsf(y)) * (oldX >= 0.0f ? 1.0f : -1.0f);
                        y = (1.0f - fabsf(oldX)) * (y >= 0.0f ? 1.0f : -1.0f);
                    }

                    outU = x;
                    outV = y;
                }

                __device__ inline float NormalizeCoordDevice(float value, float minValue, float maxValue)
                {
                    const float extent = maxValue - minValue;
                    if (extent <= 1e-12f)
                    {
                        return 0.0f;
                    }

                    const float t = (value - minValue) / extent;
                    return t * 2.0f - 1.0f;
                }

                __global__ void PackInputsFromDeviceBuffersKernel(
                    const float4* worldPosition,
                    const float4* worldNormalValid,
                    float* packedInputs,
                    uint32_t inputDims,
                    uint32_t count,
                    uint32_t paddedCount,
                    bool normalizePositions,
                    float minX, float minY, float minZ,
                    float maxX, float maxY, float maxZ)
                {
                    const uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
                    if (i >= paddedCount)
                    {
                        return;
                    }

                    float* dst = packedInputs + i * inputDims;
                    for (uint32_t d = 0; d < inputDims; ++d)
                    {
                        dst[d] = 0.0f;
                    }

                    if (i >= count)
                    {
                        return;
                    }

                    const float4 pos = worldPosition[i];
                    float4 normalValid = worldNormalValid[i];
                    if (normalValid.w <= 0.5f)
                    {
                        return;
                    }

                    const float nLenSq = normalValid.x * normalValid.x + normalValid.y * normalValid.y + normalValid.z * normalValid.z;
                    if (!isfinite(nLenSq) || nLenSq <= 1e-12f)
                    {
                        return;
                    }

                    const float invLen = rsqrtf(nLenSq);
                    normalValid.x *= invLen;
                    normalValid.y *= invLen;
                    normalValid.z *= invLen;

                    float u = 0.0f;
                    float v = 0.0f;
                    OctEncodeDevice(normalValid.x, normalValid.y, normalValid.z, u, v);

                    dst[0] = normalizePositions ? NormalizeCoordDevice(pos.x, minX, maxX) : pos.x;
                    dst[1] = normalizePositions ? NormalizeCoordDevice(pos.y, minY, maxY) : pos.y;
                    dst[2] = normalizePositions ? NormalizeCoordDevice(pos.z, minZ, maxZ) : pos.z;
                    dst[3] = u;
                    dst[4] = v;
                }

                __global__ void UnpackOutputsToDeviceBufferKernel(
                    const float* inferenceOutputs,
                    uint32_t outputDims,
                    const float4* worldNormalValid,
                    float4* outRGBA,
                    float4* historyRGBA,
                    uint32_t count,
                    bool enableTemporalSmoothing,
                    float historyWeight,
                    DeviceInferenceStats* stats)
                {
                    const uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
                    if (i >= count)
                    {
                        return;
                    }

                    const float4 normalValid = worldNormalValid[i];
                    if (normalValid.w <= 0.5f)
                    {
                        const float4 zeroValue = make_float4(0.0f, 0.0f, 0.0f, 1.0f);
                        outRGBA[i] = zeroValue;
                        if (historyRGBA)
                        {
                            historyRGBA[i] = zeroValue;
                        }
                        return;
                    }

                    float3 currentValue;
                    currentValue.x = ClampNonNegativeDevice(inferenceOutputs[i * outputDims + 0]);
                    currentValue.y = ClampNonNegativeDevice(inferenceOutputs[i * outputDims + 1]);
                    currentValue.z = ClampNonNegativeDevice(inferenceOutputs[i * outputDims + 2]);

                    if (enableTemporalSmoothing && historyRGBA)
                    {
                        const float currentWeight = 1.0f - historyWeight;
                        const float4 historyValue = historyRGBA[i];
                        currentValue.x = currentValue.x * currentWeight + historyValue.x * historyWeight;
                        currentValue.y = currentValue.y * currentWeight + historyValue.y * historyWeight;
                        currentValue.z = currentValue.z * currentWeight + historyValue.z * historyWeight;
                    }

                    const float4 outValue = make_float4(currentValue.x, currentValue.y, currentValue.z, 1.0f);
                    outRGBA[i] = outValue;
                    if (historyRGBA)
                    {
                        historyRGBA[i] = outValue;
                    }

                    const float luminance = currentValue.x * 0.2126f + currentValue.y * 0.7152f + currentValue.z * 0.0722f;
                    const float maxChannel = fmaxf(currentValue.x, fmaxf(currentValue.y, currentValue.z));

                    atomicAdd(&stats->ValidCount, 1u);
                    atomicAdd(&stats->LuminanceSum, luminance);
                    atomicMax(&stats->MaxChannelBits, __float_as_uint(maxChannel));
                }

                __global__ void CompactTrainingSamplesKernel(
                    const NIVTrainingSampleGPU* rawSamples,
                    uint32_t rawCount,
                    float* packedInputs,
                    uint32_t inputDims,
                    float* packedTargets,
                    uint32_t outputDims,
                    uint32_t baseOffset,
                    uint32_t maxCount,
                    bool normalizePositions,
                    float minX, float minY, float minZ,
                    float maxX, float maxY, float maxZ,
                    uint32_t* outAcceptedCount,
                    float* outRgbSums)
                {
                    const uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
                    if (i >= rawCount)
                    {
                        return;
                    }

                    const NIVTrainingSampleGPU sample = rawSamples[i];
                    const float r = sample.DiffuseIrradiance.x;
                    const float g = sample.DiffuseIrradiance.y;
                    const float b = sample.DiffuseIrradiance.z;
                    if (r <= 1e-8f && g <= 1e-8f && b <= 1e-8f)
                    {
                        return;
                    }

                    float nx = sample.WorldNormal.x;
                    float ny = sample.WorldNormal.y;
                    float nz = sample.WorldNormal.z;
                    const float nLenSq = nx * nx + ny * ny + nz * nz;
                    if (!isfinite(nLenSq) || nLenSq <= 1e-12f)
                    {
                        return;
                    }

                    const uint32_t acceptedIndex = atomicAdd(outAcceptedCount, 1u);
                    const uint32_t dstIndex = baseOffset + acceptedIndex;
                    if (dstIndex >= maxCount)
                    {
                        return;
                    }

                    const float invLen = rsqrtf(nLenSq);
                    nx *= invLen;
                    ny *= invLen;
                    nz *= invLen;

                    float u = 0.0f;
                    float v = 0.0f;
                    OctEncodeDevice(nx, ny, nz, u, v);

                    const float px = sample.WorldPosition.x;
                    const float py = sample.WorldPosition.y;
                    const float pz = sample.WorldPosition.z;
                    packedInputs[0 + dstIndex * inputDims] = normalizePositions ? NormalizeCoordDevice(px, minX, maxX) : px;
                    packedInputs[1 + dstIndex * inputDims] = normalizePositions ? NormalizeCoordDevice(py, minY, maxY) : py;
                    packedInputs[2 + dstIndex * inputDims] = normalizePositions ? NormalizeCoordDevice(pz, minZ, maxZ) : pz;
                    packedInputs[3 + dstIndex * inputDims] = u;
                    packedInputs[4 + dstIndex * inputDims] = v;

                    packedTargets[0 + dstIndex * outputDims] = r;
                    packedTargets[1 + dstIndex * outputDims] = g;
                    packedTargets[2 + dstIndex * outputDims] = b;

                    atomicAdd(&outRgbSums[0], r);
                    atomicAdd(&outRgbSums[1], g);
                    atomicAdd(&outRgbSums[2], b);
                }

                void ReleaseInteropBuffer(InteropBuffer& interopBuffer)
                {
                    if (interopBuffer.DevicePtr)
                    {
                        cudaFree(interopBuffer.DevicePtr);
                    }

                    if (interopBuffer.ExternalMemory)
                    {
                        cudaDestroyExternalMemory(interopBuffer.ExternalMemory);
                    }

                    interopBuffer = {};
                }

                bool EnsureInteropBuffer(InteropBuffer& interopBuffer, void* sharedHandle, size_t sizeBytes)
                {
                    if (!sharedHandle || sizeBytes == 0)
                    {
                        ReleaseInteropBuffer(interopBuffer);
                        return false;
                    }

                    if (interopBuffer.SharedHandle == sharedHandle &&
                        interopBuffer.SizeBytes == sizeBytes &&
                        interopBuffer.ExternalMemory != nullptr &&
                        interopBuffer.DevicePtr != nullptr)
                    {
                        return true;
                    }

                    ReleaseInteropBuffer(interopBuffer);

                    cudaExternalMemoryHandleDesc externalMemoryDesc = {};
                    externalMemoryDesc.type = cudaExternalMemoryHandleTypeD3D12Resource;
                    externalMemoryDesc.handle.win32.handle = sharedHandle;
                    externalMemoryDesc.size = sizeBytes;
                    externalMemoryDesc.flags = cudaExternalMemoryDedicated;

                    if (cudaImportExternalMemory(&interopBuffer.ExternalMemory, &externalMemoryDesc) != cudaSuccess)
                    {
                        ReleaseInteropBuffer(interopBuffer);
                        return false;
                    }

                    cudaExternalMemoryBufferDesc bufferDesc = {};
                    bufferDesc.offset = 0;
                    bufferDesc.size = sizeBytes;

                    if (cudaExternalMemoryGetMappedBuffer(&interopBuffer.DevicePtr, interopBuffer.ExternalMemory, &bufferDesc) != cudaSuccess)
                    {
                        ReleaseInteropBuffer(interopBuffer);
                        return false;
                    }

                    interopBuffer.SharedHandle = sharedHandle;
                    interopBuffer.SizeBytes = sizeBytes;
                    return true;
                }
            }

            NIVTrainingDataProvider::NIVTrainingDataProvider()
            {
                LoadDataset(std::filesystem::path("Content") / "Training" / "irradiance_samples.bin");
            }

            NIVTrainingDataProvider::NIVTrainingDataProvider(const std::filesystem::path& datasetPath)
            {
                LoadDataset(datasetPath);
            }

            void NIVTrainingDataProvider::LoadDataset(const std::filesystem::path& datasetPath)
            {
                const std::filesystem::path resolvedPath = ResolveDatasetPath(datasetPath);
                if (resolvedPath.empty())
                {
                    throw std::runtime_error("[NIV] Failed to find training dataset (irradiance_samples.bin).");
                }

                std::ifstream file(resolvedPath, std::ios::binary);
                if (!file.is_open())
                {
                    throw std::runtime_error(std::string("[NIV] Failed to open dataset: ") + resolvedPath.string());
                }

                TrainingDatasetHeader header{};
                file.read(reinterpret_cast<char*>(&header), sizeof(header));
                if (!file.good())
                {
                    throw std::runtime_error("[NIV] Failed to read dataset header.");
                }

                if (header.Magic != 0x44505454)
                {
                    throw std::runtime_error("[NIV] Dataset magic mismatch. Expected TTPD.");
                }
                if (header.Version != 1)
                {
                    throw std::runtime_error("[NIV] Unsupported dataset version. Expected 1.");
                }
                if (header.SampleStride < sizeof(TrainingSampleCPU))
                {
                    throw std::runtime_error("[NIV] Dataset sample stride is smaller than expected sample size.");
                }
                if (header.SampleCount == 0)
                {
                    throw std::runtime_error("[NIV] Dataset contains zero samples.");
                }

                Samples.Clear();
                Samples.Reserve(header.SampleCount);
                NonNullIndices.Clear();
                NonNullIndices.Reserve(header.SampleCount);
                HighEnergyIndices.Clear();
                HighEnergyIndices.Reserve(header.SampleCount);
                double sumR = 0.0;
                double sumG = 0.0;
                double sumB = 0.0;
                double maxR = 0.0;
                double maxG = 0.0;
                double maxB = 0.0;
                MinX = MinY = MinZ = FLT_MAX;
                MaxX = MaxY = MaxZ = -FLT_MAX;
                HasBounds = false;
                std::vector<float> luminances;
                luminances.reserve(header.SampleCount);

                std::vector<char> row(header.SampleStride, 0);
                for (uint32_t i = 0; i < header.SampleCount; ++i)
                {
                    file.read(row.data(), static_cast<std::streamsize>(row.size()));
                    if (!file.good())
                    {
                        throw std::runtime_error("[NIV] Unexpected EOF while reading training samples.");
                    }

                    const auto* src = reinterpret_cast<const TrainingSampleCPU*>(row.data());
                    NIVSample dst{};
                    dst.px = src->WorldPosition[0];
                    dst.py = src->WorldPosition[1];
                    dst.pz = src->WorldPosition[2];
                    dst.nx = src->WorldNormal[0];
                    dst.ny = src->WorldNormal[1];
                    dst.nz = src->WorldNormal[2];
                    dst.ir = src->DiffuseIrradiance[0];
                    dst.ig = src->DiffuseIrradiance[1];
                    dst.ib = src->DiffuseIrradiance[2];

                    const float nLenSq = dst.nx * dst.nx + dst.ny * dst.ny + dst.nz * dst.nz;
                    if (!std::isfinite(nLenSq) || nLenSq <= 1e-12f)
                    {
                        continue;
                    }

                    const float invLen = 1.0f / std::sqrt(nLenSq);
                    dst.nx *= invLen;
                    dst.ny *= invLen;
                    dst.nz *= invLen;

                    if (!std::isfinite(dst.px) || !std::isfinite(dst.py) || !std::isfinite(dst.pz) ||
                        !std::isfinite(dst.ir) || !std::isfinite(dst.ig) || !std::isfinite(dst.ib))
                    {
                        continue;
                    }

                    const uint32_t index = static_cast<uint32_t>(Samples.Num());
                    Samples.Add(dst);
                    sumR += dst.ir;
                    sumG += dst.ig;
                    sumB += dst.ib;
                    maxR = std::max(maxR, static_cast<double>(dst.ir));
                    maxG = std::max(maxG, static_cast<double>(dst.ig));
                    maxB = std::max(maxB, static_cast<double>(dst.ib));
                    MinX = std::min(MinX, dst.px);
                    MinY = std::min(MinY, dst.py);
                    MinZ = std::min(MinZ, dst.pz);
                    MaxX = std::max(MaxX, dst.px);
                    MaxY = std::max(MaxY, dst.py);
                    MaxZ = std::max(MaxZ, dst.pz);
                    const float lum = dst.ir * 0.2126f + dst.ig * 0.7152f + dst.ib * 0.0722f;
                    luminances.push_back(lum);
                    if ((dst.ir > 1e-8f) || (dst.ig > 1e-8f) || (dst.ib > 1e-8f))
                    {
                        NonNullIndices.Add(index);
                    }
                }

                if (Samples.IsEmpty())
                {
                    throw std::runtime_error("[NIV] Dataset has no valid samples after filtering.");
                }

                std::cout << "[NIV] Loaded dataset: " << resolvedPath.string()
                          << " (valid=" << Samples.Num()
                          << ", non-null=" << NonNullIndices.Num() << ")" << std::endl;
                HasBounds = std::isfinite(MinX) && std::isfinite(MinY) && std::isfinite(MinZ) &&
                            std::isfinite(MaxX) && std::isfinite(MaxY) && std::isfinite(MaxZ);
                if (!luminances.empty())
                {
                    std::vector<float> luminancesSorted = luminances;
                    const size_t p80Index = static_cast<size_t>(0.80 * static_cast<double>(luminancesSorted.size() - 1));
                    std::nth_element(luminancesSorted.begin(), luminancesSorted.begin() + p80Index, luminancesSorted.end());
                    const float p80 = luminancesSorted[p80Index];
                    for (uint32_t i = 0; i < static_cast<uint32_t>(Samples.Num()); ++i)
                    {
                        if (luminances[i] >= p80 && luminances[i] > 1e-8f)
                        {
                            HighEnergyIndices.Add(i);
                        }
                    }
                }
                const double invCount = 1.0 / static_cast<double>(Samples.Num());
                std::cout << "[NIV] Dataset RGB stats: mean=("
                          << (sumR * invCount) << ", "
                          << (sumG * invCount) << ", "
                          << (sumB * invCount) << ") max=("
                          << maxR << ", "
                          << maxG << ", "
                          << maxB << ")" << std::endl;
                if (HasBounds)
                {
                    std::cout << "[NIV] Dataset position bounds: min=("
                              << MinX << ", " << MinY << ", " << MinZ << ") max=("
                              << MaxX << ", " << MaxY << ", " << MaxZ << ")" << std::endl;
                }
                std::cout << "[NIV] High-energy sample count: " << HighEnergyIndices.Num() << std::endl;
            }

            void NIVTrainingDataProvider::GenerateBatch(NIVSample* out, uint32_t count)
            {
                if (!out || count == 0)
                {
                    return;
                }
                if (Samples.IsEmpty())
                {
                    throw std::runtime_error("[NIV] Cannot generate batch from empty dataset.");
                }
                if (NonNullIndices.IsEmpty())
                {
                    throw std::runtime_error("[NIV] Cannot generate training batch: dataset has no non-null irradiance samples.");
                }

                static thread_local std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<uint32_t> nonNullDist(0u, static_cast<uint32_t>(NonNullIndices.Num() - 1u));
                std::uniform_int_distribution<uint32_t> highEnergyDist(
                    0u,
                    static_cast<uint32_t>((HighEnergyIndices.IsEmpty() ? NonNullIndices.Num() : HighEnergyIndices.Num()) - 1u)
                );

                for (uint32_t i = 0; i < count; ++i)
                {
                    const bool useHighEnergy = !HighEnergyIndices.IsEmpty() && ((i & 3u) == 0u);
                    const uint32_t sampleIndex = useHighEnergy
                        ? HighEnergyIndices[highEnergyDist(rng)]
                        : NonNullIndices[nonNullDist(rng)];
                    out[i] = Samples[sampleIndex];
                }
            }

            bool NIVTrainingDataProvider::GetPositionBounds(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ) const
            {
                if (!HasBounds)
                {
                    return false;
                }
                minX = MinX; minY = MinY; minZ = MinZ;
                maxX = MaxX; maxY = MaxY; maxZ = MaxZ;
                return true;
            }

            struct NIVCoach::Impl
            {
                tcnn::TrainableModel Model;
                std::unique_ptr<INIVTrainingDataProvider> OwnedProvider;
                InteropBuffer TrainingBatchInterop;
                InteropBuffer WorldPositionInterop;
                InteropBuffer WorldNormalValidInterop;
                InteropBuffer OutputInterop;
                InteropBuffer HistoryInterop;
            };

            NIVCoach::NIVCoach() : ImplPtr(std::make_unique<Impl>())
            {
            }

            NIVCoach::~NIVCoach()
            {
                if (ImplPtr)
                {
                    ReleaseInteropBuffer(ImplPtr->TrainingBatchInterop);
                    ReleaseInteropBuffer(ImplPtr->WorldPositionInterop);
                    ReleaseInteropBuffer(ImplPtr->WorldNormalValidInterop);
                    ReleaseInteropBuffer(ImplPtr->OutputInterop);
                    ReleaseInteropBuffer(ImplPtr->HistoryInterop);
                }
            }

            void NIVCoach::SetTrainingDataProvider(INIVTrainingDataProvider* provider)
            {
                Provider = provider;
                if (provider)
                {
                    ImplPtr->OwnedProvider.reset();
                }
            }

            void NIVCoach::SetCheckpointPath(const std::string& checkpointPath)
            {
                ActiveCheckpointPath = checkpointPath;
            }

            void NIVCoach::SetAutoCheckpointInterval(uint32 autoCheckpointInterval)
            {
                AutoCheckpointInterval = autoCheckpointInterval > 0 ? autoCheckpointInterval : 0;
            }

            void NIVCoach::Initialize(uint32_t inputDims, uint32_t outputDims, uint32_t batchSize, uint32_t trainingSteps, const std::string& datasetPath)
            {
                try
                {
                    std::cout << "[NIV] Initialize: start" << std::endl;
                    InputDims = inputDims;
                    OutputDims = outputDims;
                    BatchSize = batchSize;
                    TrainingSteps = trainingSteps;
                    DatasetPath = datasetPath;

                    std::cout << "[NIV] Calling cudaGetDeviceCount..." << std::endl;
                    int deviceCount = SafeCudaGetDeviceCount();
                    if (deviceCount < 0)
                    {
                        throw std::runtime_error(
                            "[NIV] CUDA driver crashed during initialization (access violation in cudaGetDeviceCount).\n"
                            "This is almost always a CUDA runtime vs driver version mismatch.\n"
                            "Fix: update your NVIDIA GPU driver to the version required by the CUDA toolkit "
                            "this project was built with (check $(CUDA_PATH) version and nvidia.com/drivers).");
                    }
                    if (deviceCount == 0)
                    {
                        throw std::runtime_error("[NIV] No CUDA-capable GPU found.");
                    }
                    std::cout << "[NIV] deviceCount=" << deviceCount << std::endl;
                    cudaError_t cudaErr = cudaSuccess;

                    std::cout << "[NIV] Calling cudaGetDevice..." << std::endl;
                    int activeDevice = 0;
                    cudaErr = cudaGetDevice(&activeDevice);
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaGetDevice failed: ") + cudaGetErrorString(cudaErr));
                    }
                    std::cout << "[NIV] activeDevice=" << activeDevice << std::endl;

                    std::cout << "[NIV] Calling cudaSetDevice(" << activeDevice << ")..." << std::endl;
                    cudaErr = cudaSetDevice(activeDevice);
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaSetDevice failed: ") + cudaGetErrorString(cudaErr));
                    }
                    std::cout << "[NIV] cudaSetDevice OK" << std::endl;

                    std::cout << "[NIV] Calling cudaGetDeviceProperties..." << std::endl;
                    cudaDeviceProp prop{};
                    cudaErr = cudaGetDeviceProperties(&prop, activeDevice);
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaGetDeviceProperties failed: ") + cudaGetErrorString(cudaErr));
                    }

                    std::cout << "[NIV] CUDA device " << activeDevice << ": " << prop.name << " (cc " << prop.major << "." << prop.minor << ")" << std::endl;

                    std::cout << "[NIV] Working directory: " << std::filesystem::current_path().string() << std::endl;
                    const std::vector<std::filesystem::path> candidatePaths =
                    {
                        std::filesystem::path("Config") / "niv.json",
                        std::filesystem::path("Source") / "WaldemEngine" / "Config" / "Coach" / "niv.json",
                        std::filesystem::path("..") / "Source" / "WaldemEngine" / "Config" / "Coach" / "niv.json",
                        std::filesystem::path("..") / ".." / "Source" / "WaldemEngine" / "Config" / "Coach" / "niv.json",
                        std::filesystem::path("..") / ".." / ".." / "Source" / "WaldemEngine" / "Config" / "Coach" / "niv.json"
                    };

                    std::filesystem::path resolvedConfigPath;
                    for (const auto& candidate : candidatePaths)
                    {
                        const auto absolute = std::filesystem::absolute(candidate);
                        std::cout << "[NIV] Probe config path: " << absolute.string() << std::endl;
                        if (std::filesystem::exists(absolute))
                        {
                            resolvedConfigPath = absolute;
                            break;
                        }
                    }

                    if (resolvedConfigPath.empty())
                    {
                        throw std::runtime_error("[NIV] Failed to find niv.json in known locations.");
                    }

                    std::cout << "[NIV] Using config: " << resolvedConfigPath.string() << std::endl;

                    std::ifstream f(resolvedConfigPath);
                    if (!f.is_open())
                    {
                        throw std::runtime_error(std::string("[NIV] Found config but failed to open: ") + resolvedConfigPath.string());
                    }

                    nlohmann::json config;
                    try
                    {
                        f >> config;
                    }
                    catch (const std::exception& e)
                    {
                        throw std::runtime_error(std::string("[NIV] JSON parse failed: ") + e.what());
                    }

                    std::cout << "[NIV] JSON keys:";
                    for (auto it = config.begin(); it != config.end(); ++it)
                    {
                        std::cout << " " << it.key();
                    }
                    std::cout << std::endl;
                    if (config.contains("loss") && config["loss"].contains("otype"))
                    {
                        std::cout << "[NIV] Config loss otype: " << config["loss"]["otype"].get<std::string>() << std::endl;
                    }

                    std::cout << "[NIV] Calling create_from_config..." << std::endl;
                    ImplPtr->Model = tcnn::create_from_config(InputDims, OutputDims, config);
                    std::cout << "[NIV] create_from_config done" << std::endl;
                    cudaErr = cudaDeviceSynchronize();
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaDeviceSynchronize after create_from_config failed: ") + cudaGetErrorString(cudaErr));
                    }

                    if (!ActiveCheckpointPath.empty())
                    {
                        std::cout << "[NIV] Checking for checkpoint: " << ActiveCheckpointPath.string() << std::endl;
                        const std::filesystem::path resolvedCheckpoint = ResolveCheckpointReadPath(ActiveCheckpointPath);
                        if (!resolvedCheckpoint.empty())
                        {
                            std::cout << "[NIV] Auto-loading checkpoint: " << resolvedCheckpoint.string() << std::endl;
                            LoadCheckpoint(ActiveCheckpointPath.string());
                        }
                        else
                        {
                            std::cout << "[NIV] No checkpoint found at initialize path: " << ActiveCheckpointPath.string() << std::endl;
                        }
                    }

                    std::cout << "[NIV] Initialize: finished" << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[NIV] Initialize exception: " << e.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    std::cerr << "[NIV] Initialize unknown exception." << std::endl;
                    throw;
                }
            }

            static inline void OctEncode(const float nx, const float ny, const float nz, float& outU, float& outV)
            {
                float ax = std::abs(nx), ay = std::abs(ny), az = std::abs(nz);
                float invL1 = 1.0f / (ax + ay + az + 1e-20f);

                float ox = nx * invL1;
                float oy = ny * invL1;

                if (nz < 0.0f) {
                    float oldX = ox, oldY = oy;
                    ox = (1.0f - std::abs(oldY)) * (oldX >= 0.0f ? 1.0f : -1.0f);
                    oy = (1.0f - std::abs(oldX)) * (oldY >= 0.0f ? 1.0f : -1.0f);
                }

                outU = ox; // keep [-1,1]
                outV = oy;
            }

            void NIVCoach::SaveCheckpoint(const std::string& checkpointPath)
            {
                if (checkpointPath.empty())
                {
                    throw std::runtime_error("[NIV] SaveCheckpoint: empty path.");
                }

                const std::filesystem::path absolutePath = ResolveCheckpointWritePath(std::filesystem::path(checkpointPath));
                if (absolutePath.empty())
                {
                    throw std::runtime_error("[NIV] SaveCheckpoint: failed to resolve output path.");
                }
                if (!absolutePath.parent_path().empty())
                {
                    std::filesystem::create_directories(absolutePath.parent_path());
                }

                nlohmann::json checkpoint = ImplPtr->Model.trainer->serialize(false);
                checkpoint["_niv_meta"]["has_position_normalization"] = HasPositionNormalization;
                checkpoint["_niv_meta"]["position_min"] = { PositionMinX, PositionMinY, PositionMinZ };
                checkpoint["_niv_meta"]["position_max"] = { PositionMaxX, PositionMaxY, PositionMaxZ };
                const std::vector<uint8_t> bytes = nlohmann::json::to_cbor(checkpoint);

                std::ofstream outFile(absolutePath, std::ios::binary | std::ios::trunc);
                if (!outFile.is_open())
                {
                    throw std::runtime_error(std::string("[NIV] SaveCheckpoint: failed to open file: ") + absolutePath.string());
                }

                outFile.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
                if (!outFile.good())
                {
                    throw std::runtime_error(std::string("[NIV] SaveCheckpoint: failed to write file: ") + absolutePath.string());
                }

                ActiveCheckpointPath = absolutePath;
                std::cout << "[NIV] Checkpoint saved: " << absolutePath.string() << std::endl;
            }

            void NIVCoach::LoadCheckpoint(const std::string& checkpointPath)
            {
                if (checkpointPath.empty())
                {
                    throw std::runtime_error("[NIV] LoadCheckpoint: empty path.");
                }

                const std::filesystem::path resolved = ResolveCheckpointReadPath(std::filesystem::path(checkpointPath));
                if (resolved.empty())
                {
                    throw std::runtime_error(std::string("[NIV] LoadCheckpoint: file not found: ") + checkpointPath);
                }

                std::ifstream inFile(resolved, std::ios::binary);
                if (!inFile.is_open())
                {
                    throw std::runtime_error(std::string("[NIV] LoadCheckpoint: failed to open file: ") + resolved.string());
                }

                inFile.seekg(0, std::ios::end);
                const std::streamoff size = inFile.tellg();
                if (size <= 0)
                {
                    throw std::runtime_error(std::string("[NIV] LoadCheckpoint: empty file: ") + resolved.string());
                }
                inFile.seekg(0, std::ios::beg);

                std::vector<uint8_t> bytes(static_cast<size_t>(size));
                inFile.read(reinterpret_cast<char*>(bytes.data()), size);
                if (!inFile.good())
                {
                    throw std::runtime_error(std::string("[NIV] LoadCheckpoint: failed to read file: ") + resolved.string());
                }

                nlohmann::json checkpoint = nlohmann::json::from_cbor(bytes);
                ImplPtr->Model.trainer->deserialize(checkpoint);
                ActiveCheckpointPath = resolved;
                HasPositionNormalization = false;
                if (checkpoint.contains("_niv_meta"))
                {
                    const auto& meta = checkpoint["_niv_meta"];
                    if (meta.contains("has_position_normalization"))
                    {
                        HasPositionNormalization = meta["has_position_normalization"].get<bool>();
                    }
                    if (HasPositionNormalization && meta.contains("position_min") && meta.contains("position_max"))
                    {
                        const auto& pmin = meta["position_min"];
                        const auto& pmax = meta["position_max"];
                        if (pmin.is_array() && pmax.is_array() && pmin.size() >= 3 && pmax.size() >= 3)
                        {
                            PositionMinX = pmin[0].get<float>();
                            PositionMinY = pmin[1].get<float>();
                            PositionMinZ = pmin[2].get<float>();
                            PositionMaxX = pmax[0].get<float>();
                            PositionMaxY = pmax[1].get<float>();
                            PositionMaxZ = pmax[2].get<float>();
                        }
                    }
                }

                const cudaError_t cudaErr = cudaDeviceSynchronize();
                if (cudaErr != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] LoadCheckpoint: cudaDeviceSynchronize failed: ") + cudaGetErrorString(cudaErr));
                }

                std::cout << "[NIV] Checkpoint loaded: " << resolved.string() << std::endl;
            }

            void NIVCoach::InferIrradiance(float px, float py, float pz, float nx, float ny, float nz, float& outR, float& outG, float& outB)
            {
                if (InputDims < 5 || OutputDims < 3)
                {
                    throw std::runtime_error("[NIV] InferIrradiance requires initialized model with InputDims>=5 and OutputDims>=3.");
                }

                const float nLenSq = nx * nx + ny * ny + nz * nz;
                if (!std::isfinite(nLenSq) || nLenSq <= 1e-12f)
                {
                    throw std::runtime_error("[NIV] InferIrradiance received invalid normal.");
                }

                const float invLen = 1.0f / std::sqrt(nLenSq);
                nx *= invLen;
                ny *= invLen;
                nz *= invLen;

                float u = 0.0f;
                float v = 0.0f;
                OctEncode(nx, ny, nz, u, v);

                const uint32_t inferenceBatch = static_cast<uint32_t>(tcnn::BATCH_SIZE_GRANULARITY);
                tcnn::GPUMatrix<float> inferenceInputs(InputDims, inferenceBatch);
                tcnn::GPUMatrix<float> inferenceOutputs(OutputDims, inferenceBatch);

                std::vector<float> hInputs(InputDims * inferenceBatch, 0.0f);
                std::vector<float> hOutputs(OutputDims * inferenceBatch, 0.0f);

                const float inPx = HasPositionNormalization ? NormalizeCoord(px, PositionMinX, PositionMaxX) : px;
                const float inPy = HasPositionNormalization ? NormalizeCoord(py, PositionMinY, PositionMaxY) : py;
                const float inPz = HasPositionNormalization ? NormalizeCoord(pz, PositionMinZ, PositionMaxZ) : pz;
                hInputs[0] = inPx;
                hInputs[1] = inPy;
                hInputs[2] = inPz;
                hInputs[3] = u;
                hInputs[4] = v;

                cudaError_t err = cudaMemcpy(inferenceInputs.data(), hInputs.data(), hInputs.size() * sizeof(float), cudaMemcpyHostToDevice);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferIrradiance: cudaMemcpy input failed: ") + cudaGetErrorString(err));
                }

                ImplPtr->Model.network->inference(inferenceInputs, inferenceOutputs, true);

                err = cudaMemcpy(hOutputs.data(), inferenceOutputs.data(), hOutputs.size() * sizeof(float), cudaMemcpyDeviceToHost);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferIrradiance: cudaMemcpy output failed: ") + cudaGetErrorString(err));
                }

                const cudaError_t cudaErr = cudaDeviceSynchronize();
                if (cudaErr != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferIrradiance: cudaDeviceSynchronize failed: ") + cudaGetErrorString(cudaErr));
                }

                outR = ClampNonNegative(hOutputs[0]);
                outG = ClampNonNegative(hOutputs[1]);
                outB = ClampNonNegative(hOutputs[2]);
            }

            void NIVCoach::InferPackedBatch(const float* packedPositionNormal, uint32 count, float* outRGBA)
            {
                if (!packedPositionNormal || !outRGBA || count == 0)
                {
                    return;
                }
                if (InputDims < 5 || OutputDims < 3)
                {
                    throw std::runtime_error("[NIV] InferPackedBatch requires initialized model with InputDims>=5 and OutputDims>=3.");
                }

                const uint32 batchGranularity = static_cast<uint32>(tcnn::BATCH_SIZE_GRANULARITY);
                const uint32 paddedCount = ((count + batchGranularity - 1u) / batchGranularity) * batchGranularity;

                tcnn::GPUMatrix<float> inferenceInputs(InputDims, paddedCount);
                tcnn::GPUMatrix<float> inferenceOutputs(OutputDims, paddedCount);

                std::vector<float> hInputs(InputDims * paddedCount, 0.0f);
                std::vector<float> hOutputs(OutputDims * paddedCount, 0.0f);

                for (uint32 i = 0; i < count; ++i)
                {
                    const float px = packedPositionNormal[i * 8 + 0];
                    const float py = packedPositionNormal[i * 8 + 1];
                    const float pz = packedPositionNormal[i * 8 + 2];
                    float nx = packedPositionNormal[i * 8 + 4];
                    float ny = packedPositionNormal[i * 8 + 5];
                    float nz = packedPositionNormal[i * 8 + 6];
                    const float valid = packedPositionNormal[i * 8 + 7];

                    if (valid <= 0.5f)
                    {
                        continue;
                    }

                    const float nLenSq = nx * nx + ny * ny + nz * nz;
                    if (!std::isfinite(nLenSq) || nLenSq <= 1e-12f)
                    {
                        continue;
                    }

                    const float invLen = 1.0f / std::sqrt(nLenSq);
                    nx *= invLen;
                    ny *= invLen;
                    nz *= invLen;

                    float u = 0.0f;
                    float v = 0.0f;
                    OctEncode(nx, ny, nz, u, v);

                    const float inPx = HasPositionNormalization ? NormalizeCoord(px, PositionMinX, PositionMaxX) : px;
                    const float inPy = HasPositionNormalization ? NormalizeCoord(py, PositionMinY, PositionMaxY) : py;
                    const float inPz = HasPositionNormalization ? NormalizeCoord(pz, PositionMinZ, PositionMaxZ) : pz;
                    hInputs[0 + i * InputDims] = inPx;
                    hInputs[1 + i * InputDims] = inPy;
                    hInputs[2 + i * InputDims] = inPz;
                    hInputs[3 + i * InputDims] = u;
                    hInputs[4 + i * InputDims] = v;
                }

                cudaError_t err = cudaMemcpy(inferenceInputs.data(), hInputs.data(), hInputs.size() * sizeof(float), cudaMemcpyHostToDevice);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferPackedBatch: cudaMemcpy input failed: ") + cudaGetErrorString(err));
                }

                ImplPtr->Model.network->inference(inferenceInputs, inferenceOutputs, true);

                err = cudaMemcpy(hOutputs.data(), inferenceOutputs.data(), hOutputs.size() * sizeof(float), cudaMemcpyDeviceToHost);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferPackedBatch: cudaMemcpy output failed: ") + cudaGetErrorString(err));
                }

                const cudaError_t cudaErr = cudaDeviceSynchronize();
                if (cudaErr != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferPackedBatch: cudaDeviceSynchronize failed: ") + cudaGetErrorString(cudaErr));
                }

                for (uint32 i = 0; i < count; ++i)
                {
                    const float valid = packedPositionNormal[i * 8 + 7];
                    if (valid > 0.5f)
                    {
                        outRGBA[i * 4 + 0] = ClampNonNegative(hOutputs[0 + i * OutputDims]);
                        outRGBA[i * 4 + 1] = ClampNonNegative(hOutputs[1 + i * OutputDims]);
                        outRGBA[i * 4 + 2] = ClampNonNegative(hOutputs[2 + i * OutputDims]);
                        outRGBA[i * 4 + 3] = 1.0f;
                    }
                    else
                    {
                        outRGBA[i * 4 + 0] = 0.0f;
                        outRGBA[i * 4 + 1] = 0.0f;
                        outRGBA[i * 4 + 2] = 0.0f;
                        outRGBA[i * 4 + 3] = 1.0f;
                    }
                }
            }

            void NIVCoach::InferFromBuffers(const float* worldPositionRGBA, const float* worldNormalValidRGBA, uint32 count, float* outRGBA)
            {
                if (!worldPositionRGBA || !worldNormalValidRGBA || !outRGBA || count == 0)
                {
                    return;
                }
                if (InputDims < 5 || OutputDims < 3)
                {
                    throw std::runtime_error("[NIV] InferFromBuffers requires initialized model with InputDims>=5 and OutputDims>=3.");
                }

                const uint32 batchGranularity = static_cast<uint32>(tcnn::BATCH_SIZE_GRANULARITY);
                const uint32 paddedCount = ((count + batchGranularity - 1u) / batchGranularity) * batchGranularity;

                tcnn::GPUMatrix<float> inferenceInputs(InputDims, paddedCount);
                tcnn::GPUMatrix<float> inferenceOutputs(OutputDims, paddedCount);

                std::vector<float> hInputs(InputDims * paddedCount, 0.0f);
                std::vector<float> hOutputs(OutputDims * paddedCount, 0.0f);

                for (uint32 i = 0; i < count; ++i)
                {
                    const float px = worldPositionRGBA[i * 4 + 0];
                    const float py = worldPositionRGBA[i * 4 + 1];
                    const float pz = worldPositionRGBA[i * 4 + 2];
                    float nx = worldNormalValidRGBA[i * 4 + 0];
                    float ny = worldNormalValidRGBA[i * 4 + 1];
                    float nz = worldNormalValidRGBA[i * 4 + 2];
                    const float valid = worldNormalValidRGBA[i * 4 + 3];

                    if (valid <= 0.5f)
                    {
                        continue;
                    }

                    const float nLenSq = nx * nx + ny * ny + nz * nz;
                    if (!std::isfinite(nLenSq) || nLenSq <= 1e-12f)
                    {
                        continue;
                    }

                    const float invLen = 1.0f / std::sqrt(nLenSq);
                    nx *= invLen;
                    ny *= invLen;
                    nz *= invLen;

                    float u = 0.0f;
                    float v = 0.0f;
                    OctEncode(nx, ny, nz, u, v);

                    const float inPx = HasPositionNormalization ? NormalizeCoord(px, PositionMinX, PositionMaxX) : px;
                    const float inPy = HasPositionNormalization ? NormalizeCoord(py, PositionMinY, PositionMaxY) : py;
                    const float inPz = HasPositionNormalization ? NormalizeCoord(pz, PositionMinZ, PositionMaxZ) : pz;
                    hInputs[0 + i * InputDims] = inPx;
                    hInputs[1 + i * InputDims] = inPy;
                    hInputs[2 + i * InputDims] = inPz;
                    hInputs[3 + i * InputDims] = u;
                    hInputs[4 + i * InputDims] = v;
                }

                cudaError_t err = cudaMemcpy(inferenceInputs.data(), hInputs.data(), hInputs.size() * sizeof(float), cudaMemcpyHostToDevice);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromBuffers: cudaMemcpy input failed: ") + cudaGetErrorString(err));
                }

                ImplPtr->Model.network->inference(inferenceInputs, inferenceOutputs, true);

                err = cudaMemcpy(hOutputs.data(), inferenceOutputs.data(), hOutputs.size() * sizeof(float), cudaMemcpyDeviceToHost);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromBuffers: cudaMemcpy output failed: ") + cudaGetErrorString(err));
                }

                const cudaError_t cudaErr = cudaDeviceSynchronize();
                if (cudaErr != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromBuffers: cudaDeviceSynchronize failed: ") + cudaGetErrorString(cudaErr));
                }

                for (uint32 i = 0; i < count; ++i)
                {
                    const float valid = worldNormalValidRGBA[i * 4 + 3];
                    if (valid > 0.5f)
                    {
                        outRGBA[i * 4 + 0] = ClampNonNegative(hOutputs[0 + i * OutputDims]);
                        outRGBA[i * 4 + 1] = ClampNonNegative(hOutputs[1 + i * OutputDims]);
                        outRGBA[i * 4 + 2] = ClampNonNegative(hOutputs[2 + i * OutputDims]);
                        outRGBA[i * 4 + 3] = 1.0f;
                    }
                    else
                    {
                        outRGBA[i * 4 + 0] = 0.0f;
                        outRGBA[i * 4 + 1] = 0.0f;
                        outRGBA[i * 4 + 2] = 0.0f;
                        outRGBA[i * 4 + 3] = 1.0f;
                    }
                }
            }

            bool NIVCoach::InferFromSharedBuffers(
                void* sharedWorldPositionHandle,
                size_t worldPositionBytes,
                void* sharedWorldNormalValidHandle,
                size_t worldNormalValidBytes,
                void* sharedOutputHandle,
                size_t outputBytes,
                void* sharedHistoryHandle,
                size_t historyBytes,
                uint32 count,
                bool enableTemporalSmoothing,
                float historyWeight,
                uint32* outValidPixelCount,
                float* outMeanLuminance,
                float* outMaxChannel)
            {
                if (!sharedWorldPositionHandle || !sharedWorldNormalValidHandle || !sharedOutputHandle || count == 0)
                {
                    return false;
                }

                if (InputDims < 5 || OutputDims < 3)
                {
                    throw std::runtime_error("[NIV] InferFromSharedBuffers requires initialized model with InputDims>=5 and OutputDims>=3.");
                }

                const uint32 batchGranularity = static_cast<uint32>(tcnn::BATCH_SIZE_GRANULARITY);
                const uint32 paddedCount = ((count + batchGranularity - 1u) / batchGranularity) * batchGranularity;

                if (!EnsureInteropBuffer(ImplPtr->WorldPositionInterop, sharedWorldPositionHandle, worldPositionBytes) ||
                    !EnsureInteropBuffer(ImplPtr->WorldNormalValidInterop, sharedWorldNormalValidHandle, worldNormalValidBytes) ||
                    !EnsureInteropBuffer(ImplPtr->OutputInterop, sharedOutputHandle, outputBytes))
                {
                    return false;
                }

                float4* historyPtr = nullptr;
                if (enableTemporalSmoothing && sharedHistoryHandle && historyBytes >= outputBytes)
                {
                    if (EnsureInteropBuffer(ImplPtr->HistoryInterop, sharedHistoryHandle, historyBytes))
                    {
                        historyPtr = reinterpret_cast<float4*>(ImplPtr->HistoryInterop.DevicePtr);
                    }
                }
                else
                {
                    ReleaseInteropBuffer(ImplPtr->HistoryInterop);
                }

                tcnn::GPUMatrix<float> inferenceInputs(InputDims, paddedCount);
                tcnn::GPUMatrix<float> inferenceOutputs(OutputDims, paddedCount);

                const dim3 blockSize(256, 1, 1);
                const dim3 packGrid((paddedCount + blockSize.x - 1) / blockSize.x, 1, 1);
                PackInputsFromDeviceBuffersKernel<<<packGrid, blockSize>>>(
                    reinterpret_cast<const float4*>(ImplPtr->WorldPositionInterop.DevicePtr),
                    reinterpret_cast<const float4*>(ImplPtr->WorldNormalValidInterop.DevicePtr),
                    inferenceInputs.data(),
                    InputDims,
                    count,
                    paddedCount,
                    HasPositionNormalization,
                    PositionMinX, PositionMinY, PositionMinZ,
                    PositionMaxX, PositionMaxY, PositionMaxZ
                );

                cudaError_t err = cudaGetLastError();
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromSharedBuffers: input pack kernel failed: ") + cudaGetErrorString(err));
                }

                ImplPtr->Model.network->inference(inferenceInputs, inferenceOutputs, true);

                DeviceInferenceStats* deviceStats = nullptr;
                err = cudaMalloc(&deviceStats, sizeof(DeviceInferenceStats));
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromSharedBuffers: cudaMalloc stats failed: ") + cudaGetErrorString(err));
                }

                DeviceInferenceStats zeroStats = {};
                err = cudaMemcpy(deviceStats, &zeroStats, sizeof(DeviceInferenceStats), cudaMemcpyHostToDevice);
                if (err != cudaSuccess)
                {
                    cudaFree(deviceStats);
                    throw std::runtime_error(std::string("[NIV] InferFromSharedBuffers: cudaMemcpy stats init failed: ") + cudaGetErrorString(err));
                }

                const dim3 unpackGrid((count + blockSize.x - 1) / blockSize.x, 1, 1);
                UnpackOutputsToDeviceBufferKernel<<<unpackGrid, blockSize>>>(
                    inferenceOutputs.data(),
                    OutputDims,
                    reinterpret_cast<const float4*>(ImplPtr->WorldNormalValidInterop.DevicePtr),
                    reinterpret_cast<float4*>(ImplPtr->OutputInterop.DevicePtr),
                    historyPtr,
                    count,
                    enableTemporalSmoothing && historyPtr != nullptr,
                    std::clamp(historyWeight, 0.0f, 0.98f),
                    deviceStats
                );

                err = cudaGetLastError();
                if (err != cudaSuccess)
                {
                    cudaFree(deviceStats);
                    throw std::runtime_error(std::string("[NIV] InferFromSharedBuffers: output unpack kernel failed: ") + cudaGetErrorString(err));
                }

                DeviceInferenceStats hostStats = {};
                err = cudaMemcpy(&hostStats, deviceStats, sizeof(DeviceInferenceStats), cudaMemcpyDeviceToHost);
                cudaFree(deviceStats);
                if (err != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromSharedBuffers: cudaMemcpy stats readback failed: ") + cudaGetErrorString(err));
                }

                const cudaError_t cudaErr = cudaDeviceSynchronize();
                if (cudaErr != cudaSuccess)
                {
                    throw std::runtime_error(std::string("[NIV] InferFromSharedBuffers: cudaDeviceSynchronize failed: ") + cudaGetErrorString(cudaErr));
                }

                if (outValidPixelCount)
                {
                    *outValidPixelCount = hostStats.ValidCount;
                }
                if (outMeanLuminance)
                {
                    *outMeanLuminance = hostStats.ValidCount > 0 ? (hostStats.LuminanceSum / (float)hostStats.ValidCount) : 0.0f;
                }
                if (outMaxChannel)
                {
                    union
                    {
                        uint32_t U;
                        float F;
                    } maxValueBits = { hostStats.MaxChannelBits };
                    *outMaxChannel = maxValueBits.F;
                }

                return true;
            }

            // void NIVCoach::Train()
            // {
            //     try
            //     {
            //         std::cout << "[NIV] Train: start" << std::endl;
            //         std::cout << "[NIV] dims in/out=" << InputDims << "/" << OutputDims << ", batch=" << BatchSize << ", steps=" << TrainingSteps << std::endl;
            //
            //         if (BatchSize % tcnn::BATCH_SIZE_GRANULARITY != 0)
            //         {
            //             throw std::runtime_error("[NIV] BatchSize must be a multiple of BATCH_SIZE_GRANULARITY (" + std::to_string(tcnn::BATCH_SIZE_GRANULARITY) + "). Current BatchSize=" + std::to_string(BatchSize));
            //         }
            //
            //         tcnn::GPUMatrix<float> training_batch_inputs(InputDims, BatchSize);
            //         tcnn::GPUMatrix<float> training_batch_targets(OutputDims, BatchSize);
            //
            //         for (uint32_t i = 0; i < TrainingSteps; ++i)
            //         {
            //             auto ctx = ImplPtr->Model.trainer->training_step(training_batch_inputs, training_batch_targets);
            //             float loss = ImplPtr->Model.trainer->loss(*ctx);
            //             std::cout << "[NIV] iteration=" << i << " loss=" << loss << std::endl;
            //         }
            //
            //         const cudaError_t cudaErr = cudaDeviceSynchronize();
            //         if (cudaErr != cudaSuccess)
            //         {
            //             throw std::runtime_error(std::string("[NIV] cudaDeviceSynchronize after training failed: ") + cudaGetErrorString(cudaErr));
            //         }
            //
            //         std::cout << "[NIV] Train: finished" << std::endl;
            //     }
            //     catch (const std::exception& e)
            //     {
            //         std::cerr << "[NIV] Train exception: " << e.what() << std::endl;
            //         throw;
            //     }
            //     catch (...)
            //     {
            //         std::cerr << "[NIV] Train unknown exception." << std::endl;
            //         throw;
            //     }
            // }

            void NIVCoach::Train()
            {
                try
                {
                    std::cout << "[NIV] Train: start" << std::endl;
                    std::cout << "[NIV] dims in/out=" << InputDims << "/" << OutputDims << ", batch=" << BatchSize << ", steps=" << TrainingSteps << std::endl;

                    if (!Provider)
                    {
                        if (!DatasetPath.empty())
                        {
                            ImplPtr->OwnedProvider = std::make_unique<NIVTrainingDataProvider>(DatasetPath);
                        }
                        else
                        {
                            ImplPtr->OwnedProvider = std::make_unique<NIVTrainingDataProvider>();
                        }
                        Provider = ImplPtr->OwnedProvider.get();
                        std::cout << "[NIV] No provider injected; using default NIVTrainingDataProvider";
                        if (!DatasetPath.empty())
                        {
                            std::cout << " (dataset=" << DatasetPath.string() << ")";
                        }
                        std::cout << "." << std::endl;
                    }
                    if (Provider)
                    {
                        float minX, minY, minZ, maxX, maxY, maxZ;
                        if (Provider->GetPositionBounds(minX, minY, minZ, maxX, maxY, maxZ))
                        {
                            PositionMinX = minX; PositionMinY = minY; PositionMinZ = minZ;
                            PositionMaxX = maxX; PositionMaxY = maxY; PositionMaxZ = maxZ;
                            HasPositionNormalization = true;
                            std::cout << "[NIV] Position normalization enabled. min=("
                                      << PositionMinX << ", " << PositionMinY << ", " << PositionMinZ
                                      << ") max=(" << PositionMaxX << ", " << PositionMaxY << ", " << PositionMaxZ << ")" << std::endl;
                        }
                    }

                    if (InputDims != 5)
                    {
                        std::cerr << "[NIV] Warning: Paper-style NIV expects 5D input (pos3 + octNormal2)." << std::endl;
                    }
                    if (TrainingSteps < 20000)
                    {
                        std::cerr << "[NIV] Warning: paper-style NIV typically needs >10k steps and often up to 50k. Current steps=" << TrainingSteps << std::endl;
                    }
                    if (InputDims < 5)
                    {
                        throw std::runtime_error("[NIV] InputDims must be >= 5 (pos3 + octNormal2).");
                    }
                    if (OutputDims < 3)
                    {
                        throw std::runtime_error("[NIV] OutputDims must be >= 3 (irradiance RGB).");
                    }
                    if (BatchSize % tcnn::BATCH_SIZE_GRANULARITY != 0)
                    {
                        throw std::runtime_error("[NIV] BatchSize must be a multiple of BATCH_SIZE_GRANULARITY (" + std::to_string(tcnn::BATCH_SIZE_GRANULARITY) + "). Current BatchSize=" + std::to_string(BatchSize));
                    }

                    // Device matrices (column-major: [dims x batch])
                    tcnn::GPUMatrix<float> training_batch_inputs(InputDims, BatchSize);
                    tcnn::GPUMatrix<float> training_batch_targets(OutputDims, BatchSize);

                    // CPU staging buffers for file-backed / fallback providers.
                    std::vector<float> hInputs(InputDims * BatchSize);
                    std::vector<float> hTargets(OutputDims * BatchSize);
                    std::vector<NIVSample> samples(BatchSize);
                    const bool supportsSharedBatches = Provider != nullptr && Provider->SupportsSharedBatches();
                    uint32_t* deviceAcceptedCount = nullptr;
                    float* deviceRgbSums = nullptr;
                    cudaError_t err = cudaSuccess;

                    if (supportsSharedBatches)
                    {
                        err = cudaMalloc(&deviceAcceptedCount, sizeof(uint32_t));
                        if (err != cudaSuccess)
                        {
                            throw std::runtime_error(std::string("[NIV] cudaMalloc deviceAcceptedCount failed: ") + cudaGetErrorString(err));
                        }

                        err = cudaMalloc(&deviceRgbSums, sizeof(float) * 3);
                        if (err != cudaSuccess)
                        {
                            cudaFree(deviceAcceptedCount);
                            throw std::runtime_error(std::string("[NIV] cudaMalloc deviceRgbSums failed: ") + cudaGetErrorString(err));
                        }
                    }

                    for (uint32_t step = 0; step < TrainingSteps; ++step)
                    {
                        double batchSumR = 0.0;
                        double batchSumG = 0.0;
                        double batchSumB = 0.0;
                        bool filledOnGpu = false;

                        if (supportsSharedBatches)
                        {
                            uint32_t filledCount = 0;
                            uint32_t attempts = 0;
                            const uint32_t maxAttempts = 64;

                            while (filledCount < BatchSize && attempts < maxAttempts)
                            {
                                void* sharedHandle = nullptr;
                                size_t sizeBytes = 0;
                                uint32_t sampleStride = 0;
                                if (!Provider->GenerateBatchShared(BatchSize, sharedHandle, sizeBytes, sampleStride))
                                {
                                    break;
                                }

                                if (sampleStride != sizeof(NIVTrainingSampleGPU))
                                {
                                    throw std::runtime_error("[NIV] Shared training batch stride mismatch.");
                                }

                                if (!EnsureInteropBuffer(ImplPtr->TrainingBatchInterop, sharedHandle, sizeBytes))
                                {
                                    throw std::runtime_error("[NIV] Failed to import shared training batch buffer into CUDA.");
                                }

                                err = cudaMemset(deviceAcceptedCount, 0, sizeof(uint32_t));
                                if (err != cudaSuccess)
                                {
                                    throw std::runtime_error(std::string("[NIV] cudaMemset accepted count failed: ") + cudaGetErrorString(err));
                                }

                                err = cudaMemset(deviceRgbSums, 0, sizeof(float) * 3);
                                if (err != cudaSuccess)
                                {
                                    throw std::runtime_error(std::string("[NIV] cudaMemset rgb sums failed: ") + cudaGetErrorString(err));
                                }

                                const uint32_t rawCount = static_cast<uint32_t>(sizeBytes / sizeof(NIVTrainingSampleGPU));
                                const dim3 blockSize(256, 1, 1);
                                const dim3 gridSize((rawCount + blockSize.x - 1) / blockSize.x, 1, 1);
                                CompactTrainingSamplesKernel<<<gridSize, blockSize>>>(
                                    reinterpret_cast<const NIVTrainingSampleGPU*>(ImplPtr->TrainingBatchInterop.DevicePtr),
                                    rawCount,
                                    training_batch_inputs.data(),
                                    InputDims,
                                    training_batch_targets.data(),
                                    OutputDims,
                                    filledCount,
                                    BatchSize,
                                    HasPositionNormalization,
                                    PositionMinX, PositionMinY, PositionMinZ,
                                    PositionMaxX, PositionMaxY, PositionMaxZ,
                                    deviceAcceptedCount,
                                    deviceRgbSums
                                );

                                err = cudaGetLastError();
                                if (err != cudaSuccess)
                                {
                                    throw std::runtime_error(std::string("[NIV] CompactTrainingSamplesKernel failed: ") + cudaGetErrorString(err));
                                }

                                uint32_t acceptedThisAttempt = 0;
                                err = cudaMemcpy(&acceptedThisAttempt, deviceAcceptedCount, sizeof(uint32_t), cudaMemcpyDeviceToHost);
                                if (err != cudaSuccess)
                                {
                                    throw std::runtime_error(std::string("[NIV] cudaMemcpy accepted count failed: ") + cudaGetErrorString(err));
                                }

                                float rgbSums[3] = {};
                                err = cudaMemcpy(rgbSums, deviceRgbSums, sizeof(float) * 3, cudaMemcpyDeviceToHost);
                                if (err != cudaSuccess)
                                {
                                    throw std::runtime_error(std::string("[NIV] cudaMemcpy rgb sums failed: ") + cudaGetErrorString(err));
                                }

                                const uint32_t appendedCount = std::min(acceptedThisAttempt, BatchSize - filledCount);
                                filledCount += appendedCount;
                                batchSumR += rgbSums[0];
                                batchSumG += rgbSums[1];
                                batchSumB += rgbSums[2];
                                attempts++;
                            }

                            if (filledCount == BatchSize)
                            {
                                filledOnGpu = true;
                            }
                            else
                            {
                                throw std::runtime_error("[NIV] Shared live batch generation failed to fill the requested training batch.");
                            }
                        }

                        if (!filledOnGpu)
                        {
                            Provider->GenerateBatch(samples.data(), BatchSize);

                            for (uint32_t s = 0; s < BatchSize; ++s)
                            {
                                const auto& smp = samples[s];

                                float u, v;
                                OctEncode(smp.nx, smp.ny, smp.nz, u, v);

                                const float inPx = HasPositionNormalization ? NormalizeCoord(smp.px, PositionMinX, PositionMaxX) : smp.px;
                                const float inPy = HasPositionNormalization ? NormalizeCoord(smp.py, PositionMinY, PositionMaxY) : smp.py;
                                const float inPz = HasPositionNormalization ? NormalizeCoord(smp.pz, PositionMinZ, PositionMaxZ) : smp.pz;
                                hInputs[0 + s * InputDims] = inPx;
                                hInputs[1 + s * InputDims] = inPy;
                                hInputs[2 + s * InputDims] = inPz;
                                hInputs[3 + s * InputDims] = u;
                                hInputs[4 + s * InputDims] = v;

                                hTargets[0 + s * OutputDims] = smp.ir;
                                hTargets[1 + s * OutputDims] = smp.ig;
                                hTargets[2 + s * OutputDims] = smp.ib;
                                batchSumR += smp.ir;
                                batchSumG += smp.ig;
                                batchSumB += smp.ib;
                            }

                            err = cudaMemcpy(training_batch_inputs.data(), hInputs.data(), hInputs.size() * sizeof(float), cudaMemcpyHostToDevice);
                            if (err != cudaSuccess)
                            {
                                throw std::runtime_error(std::string("[NIV] cudaMemcpy inputs failed: ") + cudaGetErrorString(err));
                            }

                            err = cudaMemcpy(training_batch_targets.data(), hTargets.data(), hTargets.size() * sizeof(float), cudaMemcpyHostToDevice);
                            if (err != cudaSuccess)
                            {
                                throw std::runtime_error(std::string("[NIV] cudaMemcpy targets failed: ") + cudaGetErrorString(err));
                            }
                        }

                        // 4) Training step
                        auto ctx = ImplPtr->Model.trainer->training_step(training_batch_inputs, training_batch_targets);
                        float loss = ImplPtr->Model.trainer->loss(*ctx);

                        if ((step % 100) == 0)
                        {
                            const double invBatch = 1.0 / static_cast<double>(BatchSize);
                            std::cout << "[NIV] step=" << step
                                      << " loss=" << loss
                                      << " targetMeanRGB=("
                                      << (batchSumR * invBatch) << ", "
                                      << (batchSumG * invBatch) << ", "
                                      << (batchSumB * invBatch) << ")"
                                      << std::endl;
                        }

                        const bool shouldAutoSave = AutoCheckpointInterval > 0 &&
                            !ActiveCheckpointPath.empty() &&
                            ((step + 1) % AutoCheckpointInterval) == 0;
                        if (shouldAutoSave)
                        {
                            SaveCheckpoint(ActiveCheckpointPath.string());
                            std::cout << "[NIV] Auto-checkpoint saved at step " << (step + 1) << std::endl;
                        }
                    }

                    if (deviceAcceptedCount)
                    {
                        cudaFree(deviceAcceptedCount);
                    }
                    if (deviceRgbSums)
                    {
                        cudaFree(deviceRgbSums);
                    }

                    const cudaError_t cudaErr = cudaDeviceSynchronize();
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaDeviceSynchronize after training failed: ") + cudaGetErrorString(cudaErr));
                    }

                    if (!ActiveCheckpointPath.empty())
                    {
                        SaveCheckpoint(ActiveCheckpointPath.string());
                        std::cout << "[NIV] Final checkpoint saved after training." << std::endl;
                    }

                    std::cout << "[NIV] Train: finished" << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[NIV] Train exception: " << e.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    std::cerr << "[NIV] Train unknown exception." << std::endl;
                    throw;
                }
            }
        }
    }
}
