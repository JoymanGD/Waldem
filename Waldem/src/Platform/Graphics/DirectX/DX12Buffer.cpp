#include "wdpch.h"
#include "DX12Buffer.h"
#include "DX12Helper.h"
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    DX12Buffer::DX12Buffer(ID3D12Device* device, DX12CommandList* cmdList, BufferType type, void* data, uint32_t size) : Buffer(type, size)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = size;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&BufferResource));

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create buffer!");
        }

        if(data)
        {
            D3D12_HEAP_PROPERTIES uploadHeapProps = {};
            uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            uploadHeapProps.CreationNodeMask = 1;
            uploadHeapProps.VisibleNodeMask = 1;
            
            UINT64 uploadBufferSize;
            device->GetCopyableFootprints(&bufferDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

            D3D12_RESOURCE_DESC uploadBufferDesc = {};
            uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadBufferDesc.Width = uploadBufferSize;
            uploadBufferDesc.Height = 1;
            uploadBufferDesc.DepthOrArraySize = 1;
            uploadBufferDesc.MipLevels = 1;
            uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            uploadBufferDesc.SampleDesc.Count = 1;
            uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            
            ID3D12Resource* textureUploadHeap;
            hr = device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap));
            
            if(FAILED(hr))
            {
                DX12Helper::PrintHResultError(hr);
            }

            D3D12_SUBRESOURCE_DATA subResourceData;
            subResourceData.pData = data;
            subResourceData.RowPitch = uploadBufferSize;
            subResourceData.SlicePitch = uploadBufferSize;

            cmdList->UpdateSubresoures(BufferResource, textureUploadHeap, 1, &subResourceData);
        }

        cmdList->ResourceBarrier(BufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

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
