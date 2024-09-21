#include "wdpch.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Platform/Graphics/OpenGL/OpenGLBuffer.h"

namespace Waldem
{
    VertexBuffer* VertexBuffer::Create(void* data, uint32_t size)
    {
        switch (Renderer::RAPI)
        {
        case RendererAPI::None:
            WD_CORE_ASSERT(false, "RendererAPI 'None' is not supported!")
            return nullptr;
        case RendererAPI::OpenGL:
            return new OpenGLVertexBuffer(data, size);
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
        case RendererAPI::OpenGL:
            return new OpenGLIndexBuffer(indices, count);
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
        case RendererAPI::OpenGL:
            return new OpenGLStorageBuffer(data, size);
        default:
            WD_CORE_ASSERT(false, "The API is not supported as an Rendering API")
            return nullptr;
        }
    }
}