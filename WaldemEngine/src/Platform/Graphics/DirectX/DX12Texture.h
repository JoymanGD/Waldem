#pragma once
#include "DX12CommandList.h"
#include <d3d12.h>

namespace Waldem
{
    class WALDEM_API DX12Texture : public Texture2D
    {
    public:
        DX12Texture(WString name, ID3D12Device* device, DX12CommandList* cmdList, int width, int height, TextureFormat format, size_t dataSize, uint8_t* data);
        DX12Texture(ID3D12Device* device, DX12CommandList* cmdList, TextureDesc desc);
        void* GetPlatformResource() override { return Resource; }
        void Destroy() override { Resource->Release(); }

    private:
        ID3D12Resource* Resource;
    };
}