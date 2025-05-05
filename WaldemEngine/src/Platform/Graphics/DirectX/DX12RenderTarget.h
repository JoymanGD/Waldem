#pragma once

#include "DX12CommandList.h"
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Renderer/TextureFormat.h"

namespace Waldem
{    
    class DX12RenderTarget : public RenderTarget
    {
    public:
        DX12RenderTarget(WString name, ID3D12Device* device, DX12CommandList* cmdList, int width, int height, TextureFormat format);
        DX12RenderTarget(WString name, ID3D12Device* device, DX12CommandList* cmdList, int width, int height, TextureFormat format, ID3D12DescriptorHeap* srvHeap, uint slot);
        DX12RenderTarget(WString name, ID3D12Device* device, int width, int height, TextureFormat format, ID3D12Resource* resource);
        virtual ~DX12RenderTarget() {}
        virtual void* GetPlatformResource() override { return Resource; }
        void Destroy() override { Resource->Release(); RTVHeap->Release(); SRVHeap->Release(); }
        size_t GetPlatformShaderResourceHandle() const override { return SRVGPUHandle.ptr; }
        size_t GetPlatformRenderTargetHandle() const override { return RTVHandle.ptr;}
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() { return RTVHandle; }
        ID3D12DescriptorHeap* GetSRVHeap() { return SRVHeap; }

    private:
        ID3D12Resource* Resource;
        ID3D12DescriptorHeap* RTVHeap;
        ID3D12DescriptorHeap* SRVHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE SRVCPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE SRVGPUHandle;
    };
}