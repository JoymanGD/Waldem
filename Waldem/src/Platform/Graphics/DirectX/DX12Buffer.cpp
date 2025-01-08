#include "wdpch.h"
#include "DX12Buffer.h"

#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    DX12Buffer::DX12Buffer(ID3D12Device* device, BufferType type, void* data, uint32_t size) : Buffer(type, size)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        
        D3D12_RESOURCE_DESC indexBufferDesc = {};
        indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indexBufferDesc.Width = size;
        indexBufferDesc.Height = 1;
        indexBufferDesc.DepthOrArraySize = 1;
        indexBufferDesc.MipLevels = 1;
        indexBufferDesc.SampleDesc.Count = 1;
        indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&BufferResource));

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create index buffer!");
        }

        UINT8* pIndexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        BufferResource->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
        memcpy(pIndexDataBegin, data, size);
        BufferResource->Unmap(0, nullptr);

        switch (type)
        {
        case VertexBuffer:
            VertexBufferView.BufferLocation = BufferResource->GetGPUVirtualAddress();
            VertexBufferView.StrideInBytes = sizeof(Vertex);
            VertexBufferView.SizeInBytes = size;
            Count = VertexBufferView.SizeInBytes / VertexBufferView.StrideInBytes;
            break;
        case IndexBuffer:
            IndexBufferView.BufferLocation = BufferResource->GetGPUVirtualAddress();
            IndexBufferView.SizeInBytes = size;
            IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
            Count = IndexBufferView.SizeInBytes / sizeof(uint32_t);
            break;
        }
    }
}
