#include "wdpch.h"
#include "DX12RenderTarget.h"
#include "DX12Helper.h"

namespace Waldem
{
    DX12RenderTarget::DX12RenderTarget(String name, ID3D12Device* device, DX12CommandList* cmdList, int width, int height, TextureFormat format)
        : RenderTarget(name, width, height, format)
    {
        HRESULT hr;

        Name = name;
        
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
        textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            nullptr,
            IID_PPV_ARGS(&Resource));

        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        std::wstring widestr = std::wstring(name.begin(), name.end());
        Resource->SetName(widestr.c_str());

        // D3D12_RESOURCE_BARRIER barrier = {};
        // barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        // barrier.Transition.pResource = Resource;
        // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        // barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        // cmdList->ResourceBarrier(1, &barrier);

        //create descriptor Heap for Render Target View (RTV)
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        HRESULT h = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RTVHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 RTV Heap");
        }
        
        RenderTargetHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        
        device->CreateRenderTargetView(Resource, nullptr, RenderTargetHandle);
    }
}