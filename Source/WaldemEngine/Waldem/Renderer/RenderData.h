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
    };

    struct WALDEM_API RenderData
    {
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableAccelerationStructure TLAS;
        RenderFeatureToggles FeatureToggles;
        uint EditorViewportOutputTarget = 0;
        uint GameViewportOutputTarget = 0;
    };
}
