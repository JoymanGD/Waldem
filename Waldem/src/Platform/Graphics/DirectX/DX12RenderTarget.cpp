#include "wdpch.h"
#include "DX12RenderTarget.h"
#include "DX12Helper.h"

namespace Waldem
{
    DX12RenderTarget::DX12RenderTarget(String name, ID3D12Device* device, DX12GraphicCommandList* cmdList, int width, int height, TextureFormat format)
        : RenderTarget(name, width, height, format)
    {
        HRESULT hr;

        Name = name;
        
        if(format == TextureFormat::TEXTURE_FORMAT_D32_FLOAT)
        {
            IsDepthStencil = true;
            
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&RTVHeap));

            if(FAILED(hr))
            {
                throw std::runtime_error("Failed to create D3D12 DSV Heap");
            }
        
            D3D12_RESOURCE_DESC depthBufferDesc = {};
            depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            depthBufferDesc.Width = width;
            depthBufferDesc.Height = height;
            depthBufferDesc.DepthOrArraySize = 1;
            depthBufferDesc.MipLevels = 1;
            depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            depthBufferDesc.SampleDesc.Count = 1;
            depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;
        
            D3D12_HEAP_PROPERTIES dsvHeapProperties = {};
            dsvHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            dsvHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            dsvHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            dsvHeapProperties.CreationNodeMask = 1;
            dsvHeapProperties.VisibleNodeMask = 1;

            device->CreateCommittedResource(
                &dsvHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthBufferDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&Resource)
            );

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            RenderTargetHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
            device->CreateDepthStencilView(Resource, &dsvDesc, RenderTargetHandle);
        }
        else
        {
            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = (DXGI_FORMAT)format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            // Create the texture resource
            D3D12_HEAP_PROPERTIES heapProperties = {};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProperties.CreationNodeMask = 1;
            heapProperties.VisibleNodeMask = 1;
            
            hr = device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&Resource));

            if(FAILED(hr))
            {
                DX12Helper::PrintHResultError(hr);
            }

            std::wstring widestr = std::wstring(name.begin(), name.end());
            Resource->SetName(widestr.c_str());

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = Resource;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);

            //create descriptor Heap for Render Target View (RTV)
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = 1;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RTVHeap));

            if(FAILED(hr))
            {
                throw std::runtime_error("Failed to create D3D12 RTV Heap");
            }
            
            RenderTargetHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
            
            device->CreateRenderTargetView(Resource, nullptr, RenderTargetHandle);
        }
    }
}