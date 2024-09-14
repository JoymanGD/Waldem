#pragma once
#include "Buffer.h"

namespace Waldem
{
    class VertexArray
    {
    public:
        VertexArray() {}
        virtual ~VertexArray() {}

        virtual void Bind() const;
        virtual void Unbind() const;
        
        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer);
        virtual void SetIndexBuffer(IndexBuffer* indexBuffer);

        virtual const std::vector<VertexBuffer*> GetVertexBuffers() const = 0;
        virtual const IndexBuffer* GetIndexBuffer() const = 0;

        static VertexArray* Create();
    };
}