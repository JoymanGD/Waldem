#pragma once
#include "DX12CommandList.h"
#include "Waldem/Renderer/Buffer.h"

namespace Waldem
{
    class WALDEM_API DX12Buffer : public Buffer
    {
    public:
        DX12Buffer(ID3D12Device* device, DX12CommandList* cmdList, WString name, BufferType type, void* data, uint32_t size, uint32_t stride);
        void* GetPlatformResource() const override { return DefaultResource; }
        D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() { return IndexBufferView; }
        D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() { return VertexBufferView; }
        ID3D12Resource* GetDefaultResource() const { return DefaultResource; }
        ID3D12Resource* GetUploadResource() const { return UploadResource; }
        uint32_t GetCount() const override { return Count; }
        void SetCount(uint32_t value) override { Count = value; }
        void Destroy() override { DefaultResource->Release(); UploadResource->Release(); }

    private:
        ID3D12Resource* DefaultResource;
        ID3D12Resource* UploadResource;
        D3D12_INDEX_BUFFER_VIEW IndexBufferView;
        D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    };
}
