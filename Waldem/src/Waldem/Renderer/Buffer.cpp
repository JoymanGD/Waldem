#include "wdpch.h"
#include "Buffer.h"
#include "Renderer.h"

namespace Waldem
{
    StorageBuffer* StorageBuffer::Create(void* data, size_t size)
    {
        switch (Renderer::RAPI)
        {
        case RendererAPI::None:
            WD_CORE_ASSERT(false, "RendererAPI 'None' is not supported!")
            return nullptr;
        default:
            WD_CORE_ASSERT(false, "The API is not supported as an Rendering API")
            return nullptr;
        }
    }
}