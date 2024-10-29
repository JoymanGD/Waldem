#include "wdpch.h"
#include "DX12Buffer.h"

#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    DX12VertexBuffer::DX12VertexBuffer(ID3D12Device* device, void* data, uint32_t size)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&BufferResource));

        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        BufferResource->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        memcpy(pVertexDataBegin, data, size);
        BufferResource->Unmap(0, nullptr);

        BufferView.BufferLocation = BufferResource->GetGPUVirtualAddress();
        BufferView.StrideInBytes = sizeof(Vertex);
        BufferView.SizeInBytes = size;
    }

    DX12IndexBuffer::DX12IndexBuffer(ID3D12Device* device, void* data, uint32_t size)
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

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&BufferResource));

        UINT8* pIndexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        BufferResource->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
        memcpy(pIndexDataBegin, data, size);
        BufferResource->Unmap(0, nullptr);

        BufferView.BufferLocation = BufferResource->GetGPUVirtualAddress();
        BufferView.SizeInBytes = size;
        BufferView.Format = DXGI_FORMAT_R32_UINT;
    }
}
