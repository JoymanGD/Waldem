#include "wdpch.h"
#include "VertexArray.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Waldem
{
    VertexArray* VertexArray::Create()
    {
        auto rAPI = Renderer::GetAPI();

        switch (rAPI)
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

    void VertexArray::AddVertexBuffer(std::shared_ptr<VertexBuffer>& vertexBuffer)
    {
    }

    void VertexArray::SetIndexBuffer(std::shared_ptr<IndexBuffer>& indexBuffer)
    {
    }
}
