#pragma once
#include "Waldem/Renderer/Buffer.h"

namespace Waldem
{
    class OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(void* vertices, uint32_t size);
        ~OpenGLVertexBuffer() override;
        void Bind() const override;
        void Unbind() const override;
        const BufferLayout& GetLayout() const override { return Layout; }
        void SetLayout(const BufferLayout& layout) override { Layout = layout; }

    private:
        uint32_t BufferID;
        BufferLayout Layout;
    };
    
    class OpenGLIndexBuffer : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
        ~OpenGLIndexBuffer() override;
        void Bind() const override;
        void Unbind() const override;
        uint32_t GetCount() const override { return Count; };

    private:
        uint32_t BufferID;
        uint32_t Count;
    };
    
    class OpenGLStorageBuffer : public StorageBuffer
    {
    public:
        OpenGLStorageBuffer(void* data, size_t size);
        ~OpenGLStorageBuffer() override;
        void Bind() const override;
        void Unbind() const override;

    private:
        uint32_t BufferID;
    };
}