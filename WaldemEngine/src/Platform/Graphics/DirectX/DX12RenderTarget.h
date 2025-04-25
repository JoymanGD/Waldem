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
        virtual ~DX12RenderTarget() {}
        virtual void* GetPlatformResource() override { return Resource; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetHandle() const { return RenderTargetHandle; }
        void Destroy() override { Resource->Release(); RTVHeap->Release(); }

    private:
        ID3D12Resource* Resource;
        ID3D12DescriptorHeap* RTVHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetHandle;
    };
}