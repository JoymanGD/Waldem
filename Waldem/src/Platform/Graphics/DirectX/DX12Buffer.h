#pragma once
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"

namespace Waldem
{
    class WALDEM_API DX12VertexBuffer : VertexBuffer
    {
    public:
        DX12VertexBuffer(ID3D12Device* device, void* data, uint32_t size);

        D3D12_VERTEX_BUFFER_VIEW& GetBufferView() { return BufferView; }
        
    private:
        ID3D12Resource* BufferResource;
        D3D12_VERTEX_BUFFER_VIEW BufferView;
    };
    
    class WALDEM_API DX12IndexBuffer : IndexBuffer
    {
    public:
        DX12IndexBuffer(ID3D12Device* device, void* data, uint32_t size);

        D3D12_INDEX_BUFFER_VIEW& GetBufferView() { return BufferView; }
        
    private:
        ID3D12Resource* BufferResource;
        D3D12_INDEX_BUFFER_VIEW BufferView;
    };
}
