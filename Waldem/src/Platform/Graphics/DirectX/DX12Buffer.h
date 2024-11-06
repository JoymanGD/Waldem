#pragma once
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"

namespace Waldem
{
    class WALDEM_API DX12VertexBuffer : public VertexBuffer
    {
    public:
        DX12VertexBuffer(ID3D12Device* device, void* data, uint32_t size);

        D3D12_VERTEX_BUFFER_VIEW& GetBufferView() { return BufferView; }
        
    private:
        ID3D12Resource* BufferResource;
        D3D12_VERTEX_BUFFER_VIEW BufferView;
    };
    
    class WALDEM_API DX12IndexBuffer : public IndexBuffer
    {
    public:
        DX12IndexBuffer(ID3D12Device* device, void* data, uint32_t count);

        D3D12_INDEX_BUFFER_VIEW& GetBufferView() { return BufferView; }
        uint32_t GetCount() const override { return Count; };

    private:
        ID3D12Resource* BufferResource;
        D3D12_INDEX_BUFFER_VIEW BufferView;
        uint32_t Count;
    };
    
    class WALDEM_API DX12StorageBuffer : public StorageBuffer
    {
    public:
        DX12StorageBuffer(ID3D12Device* device, void* data, uint32_t count);
        void* GetPlatformResource() const override { return BufferResource; }

    private:
        ID3D12Resource* BufferResource;
    };
}
