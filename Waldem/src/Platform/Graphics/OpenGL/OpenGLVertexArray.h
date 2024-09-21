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
        void AddVertexBuffer(VertexBuffer* vertexBuffer) override;
        void SetIndexBuffer(IndexBuffer* indexBuffer) override;
        const std::vector<VertexBuffer*> GetVertexBuffers() const override { return VertexBuffers; }
        const IndexBuffer* GetIndexBuffer() const override { return IndexBuffer; }

    private:
        uint32_t RendererID;
        std::vector<VertexBuffer*> VertexBuffers;
        IndexBuffer* IndexBuffer;
    };
}
