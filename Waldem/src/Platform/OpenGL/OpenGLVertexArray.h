#pragma once
#include "Waldem/Renderer/VertexArray.h"
#include "Waldem/Renderer/Buffer.h"

namespace Waldem
{
    class OpenGLVertexArray : public VertexArray
    {
    public:
        OpenGLVertexArray();
        virtual ~OpenGLVertexArray();
        void Bind() const override;
        void Unbind() const override;
        void AddVertexBuffer(std::shared_ptr<VertexBuffer>& vertexBuffer) override;
        void SetIndexBuffer(std::shared_ptr<IndexBuffer>& indexBuffer) override;
        const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const override { return VertexBuffers; }
        const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const override { return IndexBuffer; }

    private:
        uint32_t RendererID;
        std::vector<std::shared_ptr<VertexBuffer>> VertexBuffers;
        std::shared_ptr<IndexBuffer> IndexBuffer;
    };
}
