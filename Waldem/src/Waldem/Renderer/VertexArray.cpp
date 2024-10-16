#include "wdpch.h"
#include "VertexArray.h"
#include "Renderer.h"
#include "Platform/Graphics/OpenGL/OpenGLVertexArray.h"

namespace Waldem
{
    VertexArray* VertexArray::Create()
    {
        switch (Renderer::RAPI)
        {
        case RendererAPI::None:
            WD_CORE_ASSERT(false, "RendererAPI 'None' is not supported!")
            return nullptr;
        case RendererAPI::OpenGL:
            return new OpenGLVertexArray();
        default:
            WD_CORE_ASSERT(false, "The API is not supported as an Rendering API")
            return nullptr;
        }
    }

    void VertexArray::Bind() const
    {
    }

    void VertexArray::Unbind() const
    {
    }

    void VertexArray::AddVertexBuffer(VertexBuffer* vertexBuffer)
    {
    }

    void VertexArray::SetIndexBuffer(IndexBuffer* indexBuffer)
    {
    }
}
