#pragma once
#include "DX12CommandList.h"
#include "Waldem/Renderer/Buffer.h"

namespace Waldem
{
    class WALDEM_API DX12Buffer : public Buffer
    {
    public:
        DX12Buffer(ID3D12Device* device, DX12CommandList* cmdList, BufferType type, void* data, uint32_t size);
        void* GetPlatformResource() const override { return BufferResource; }
        D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() { return IndexBufferView; }
        D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() { return VertexBufferView; }
        uint32_t GetCount() const override { return Count; }

    private:
        ID3D12Resource* BufferResource;
        D3D12_INDEX_BUFFER_VIEW IndexBufferView;
        D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
        uint32_t Count;
    };
}
