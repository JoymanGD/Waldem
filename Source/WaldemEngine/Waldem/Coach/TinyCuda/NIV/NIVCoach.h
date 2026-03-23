#pragma once
#include "../TinyCoach.h"
#include <filesystem>
#include <memory>
#include <string>

#include "Waldem/Types/BasicTypes.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    namespace Coach
    {
        namespace TinyCuda
        {
            struct WALDEM_API NIVSample
            {
                // world space
                float px, py, pz;
                // world space normal (unit)
                float nx, ny, nz;
                // indirect irradiance target (RGB)
                float ir, ig, ib;
            };

            struct WALDEM_API NIVTrainingSampleGPU
            {
                struct Float4
                {
                    float x, y, z, w;
                };

                Float4 WorldPosition;
                Float4 WorldNormal;
                Float4 DiffuseIrradiance;
            };

            class WALDEM_API INIVTrainingDataProvider
            {
            public:
                virtual ~INIVTrainingDataProvider() = default;

                // Fill `out` with exactly `count` samples.
                // Must implement:
                //  - uniform volume sampling
                //  - 20% surface samples with n aligned to surface normal
                //  - cull inside-geometry samples
                //  - (optional) discard null irradiance samples
                virtual void GenerateBatch(NIVSample* out, uint32 count) = 0;
                virtual bool GetPositionBounds(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ) const { return false; }
                virtual bool SupportsSharedBatches() const { return false; }
                virtual bool GenerateBatchShared(uint32 count, void*& sharedHandle, size_t& sizeBytes, uint32& sampleStride) { return false; }
            };

            class NIVTrainingDataProvider : public INIVTrainingDataProvider
            {
            public:
                NIVTrainingDataProvider();
                explicit NIVTrainingDataProvider(const std::filesystem::path& datasetPath);

                void GenerateBatch(NIVSample* out, uint32 count) override;
                bool GetPositionBounds(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ) const;

            private:
                void LoadDataset(const std::filesystem::path& datasetPath);

                WArray<NIVSample> Samples;
                WArray<uint32> NonNullIndices;
                WArray<uint32> HighEnergyIndices;
                float MinX = 0.0f, MinY = 0.0f, MinZ = 0.0f;
                float MaxX = 0.0f, MaxY = 0.0f, MaxZ = 0.0f;
                bool HasBounds = false;
            };

            class WALDEM_API NIVCoach : public TinyCoach
            {
            public:
                NIVCoach();
                ~NIVCoach();

                void Initialize(uint32 inputDims, uint32 outputDims, uint32 batchSize, uint32 trainingSteps, const std::string& datasetPath) override;
                void Train() override;
                void SetTrainingDataProvider(INIVTrainingDataProvider* provider);
                void SetCheckpointPath(const std::string& checkpointPath);
                void SetAutoCheckpointInterval(uint32 autoCheckpointInterval);
                void SaveCheckpoint(const std::string& checkpointPath);
                void LoadCheckpoint(const std::string& checkpointPath);
                void InferIrradiance(float px, float py, float pz, float nx, float ny, float nz, float& outR, float& outG, float& outB);
                void InferPackedBatch(const float* packedPositionNormal, uint32 count, float* outRGBA);
                void InferFromBuffers(const float* worldPositionRGBA, const float* worldNormalValidRGBA, uint32 count, float* outRGBA);
                bool InferFromSharedBuffers(
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
                    uint32* outValidPixelCount = nullptr,
                    float* outMeanLuminance = nullptr,
                    float* outMaxChannel = nullptr);

            private:
                INIVTrainingDataProvider* Provider = nullptr;
                struct Impl;
                std::unique_ptr<Impl> ImplPtr;
                uint32 InputDims = 1;
                uint32 OutputDims = 1;
                uint32 BatchSize = 1;
                uint32 TrainingSteps = 1;
                std::filesystem::path DatasetPath;
                std::filesystem::path ActiveCheckpointPath;
                uint32 AutoCheckpointInterval = 1000;
                float PositionMinX = 0.0f, PositionMinY = 0.0f, PositionMinZ = 0.0f;
                float PositionMaxX = 0.0f, PositionMaxY = 0.0f, PositionMaxZ = 0.0f;
                bool HasPositionNormalization = false;
            };
        }
    }
}
