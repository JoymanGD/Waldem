#include "wdpch.h"
#include "DX12Buffer.h"
#include "DX12Helper.h"

namespace Waldem
{
    DX12Buffer::DX12Buffer(ID3D12Device* device, DX12CommandList* cmdList, String name, BufferType type, void* data, uint32_t size, uint32_t stride) : Buffer(name, type, size, stride)
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
            IID_PPV_ARGS(&DefaultResource));

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create buffer!");
        }
        
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
            
        hr = device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadResource));
            
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        if(data)
        {
            D3D12_SUBRESOURCE_DATA subResourceData;
            subResourceData.pData = data;
            subResourceData.RowPitch = uploadBufferSize;
            subResourceData.SlicePitch = uploadBufferSize;

            cmdList->UpdateSubresoures(DefaultResource, UploadResource, 1, &subResourceData);
        }

        cmdList->ResourceBarrier(DefaultResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); 

        switch (type)
        {
        case VertexBuffer:
            VertexBufferView.BufferLocation = DefaultResource->GetGPUVirtualAddress();
            VertexBufferView.StrideInBytes = stride;
            VertexBufferView.SizeInBytes = size;
            Count = VertexBufferView.SizeInBytes / VertexBufferView.StrideInBytes;
            break;
        case IndexBuffer:
            IndexBufferView.BufferLocation = DefaultResource->GetGPUVirtualAddress();
            IndexBufferView.SizeInBytes = size;
            IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
            Count = IndexBufferView.SizeInBytes / sizeof(uint32_t);
            break;
        }
    }
}
