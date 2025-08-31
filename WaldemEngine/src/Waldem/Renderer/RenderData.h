#pragma once
#include "ResizableAccelerationStructure.h"
#include "ResizableBuffer.h"

namespace Waldem
{
    struct WALDEM_API RenderData
    {
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableAccelerationStructure TLAS;
    };
}
