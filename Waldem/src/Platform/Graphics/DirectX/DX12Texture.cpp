#include "wdpch.h"
#include "DX12Texture.h"

namespace Waldem
{
    DX12Texture::DX12Texture(ID3D12Device* device, std::string name, int width, int height, int channels, uint8_t* data)
    {
        Name = name;
        
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Create the texture resource
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;
        
        device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&Resource));

        if(data)
        {
            D3D12_HEAP_PROPERTIES uploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD };
            UINT64 uploadBufferSize;
            device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

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
            device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap));

            void* mappedData;
            D3D12_RANGE readRange = {};
            textureUploadHeap->Map(0, &readRange, &mappedData);

            UINT8* destData = reinterpret_cast<uint8_t*>(mappedData);
            for (UINT row = 0; row < height; ++row)
            {
                memcpy(destData + row * (width * channels), data + row * (width * channels), width * channels);
            }

            textureUploadHeap->Unmap(0, nullptr);
        }

        //TODO: transition resource to shader resource state
    }
}