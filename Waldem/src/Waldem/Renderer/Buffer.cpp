#include "wdpch.h"
#include "Buffer.h"
#include "Renderer.h"

namespace Waldem
{
    VertexBuffer* VertexBuffer::Create(void* data, uint32_t size)
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

    IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t count)
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