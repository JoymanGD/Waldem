#pragma once
#include "ResizableAccelerationStructure.h"
#include "ResizableBuffer.h"

namespace Waldem
{
    struct WALDEM_API RenderFeatureToggles
    {
        bool EnableSkyPass = true;
        bool EnableGBufferPass = true;
        bool EnableRayTracingPass = true;
        bool EnableDeferredPass = true;

        bool EnableReflections = true;
        bool EnableDirectLighting = true;
        bool EnableSpecular = true;
        bool EnableMetallic = true;

        bool EnablePathTracing = false;
        bool EnablePathTracingAccumulation = true;
        uint PathTracingMaxBounces = 3;
        uint PathTracingSamplesPerPixel = 1;
    };

    struct WALDEM_API RenderData
    {
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableAccelerationStructure TLAS;
        RenderFeatureToggles FeatureToggles;
        uint EditorViewportOutputTarget = 0;
        uint GameViewportOutputTarget = 0;

        Buffer* SharedDrawCommandsBuffer = nullptr;
        Buffer* SharedMaterialAttributesBuffer = nullptr;
        Buffer* SharedWorldTransformsBuffer = nullptr;
        Buffer* SharedLightsBuffer = nullptr;
        Buffer* SharedLightTransformsBuffer = nullptr;
        Buffer* SharedLightsIndicesBuffer = nullptr;
        uint SharedDrawCommandsCount = 0;
        uint SharedNumLights = 0;

        bool RequestTrainingDatasetCapture = false;
        uint TrainingDatasetSampleCount = 32768;
        uint TrainingDatasetRaysPerPoint = 32;
        uint TrainingDatasetCaptureBatches = 1;
        uint TrainingDatasetMaxBounces = 3;
        uint TrainingDatasetSeed = 1;
        WString TrainingDatasetOutputPath = "Content/Training/irradiance_samples.bin";
        bool TrainingDatasetDebugOutput = false;
    };
}
