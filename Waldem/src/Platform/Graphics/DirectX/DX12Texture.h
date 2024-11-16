#pragma once
#include "DX12GraphicCommandList.h"
#include <d3d12.h>

namespace Waldem
{
    class WALDEM_API DX12Texture : public Texture2D
    {
    public:
        DX12Texture(String name, ID3D12Device* device, DX12GraphicCommandList* cmdList, int width, int height, TextureFormat format, uint8_t* data);
        void* GetPlatformResource() override { return Resource; }

    private:
        ID3D12Resource* Resource;
    };
}