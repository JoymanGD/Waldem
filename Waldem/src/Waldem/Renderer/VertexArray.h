#pragma once
#include "Buffer.h"

namespace Waldem
{
    class VertexArray
    {
    public:
        VertexArray() {}
        ~VertexArray() {}

        virtual void Bind() const;
        virtual void Unbind() const;
        
        virtual void AddVertexBuffer(std::shared_ptr<VertexBuffer>& vertexBuffer);
        virtual void SetIndexBuffer(std::shared_ptr<IndexBuffer>& indexBuffer);

        virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const = 0;
        virtual const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const = 0;

        static VertexArray* Create();
    };
}