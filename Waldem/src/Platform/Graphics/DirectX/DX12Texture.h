#pragma once
#include "Waldem/Renderer/Texture.h"
#include "DX12CommandList.h"
#include <d3d12.h>

namespace Waldem
{
    class WALDEM_API DX12Texture : public Texture2D
    {
    public:
        std::string GetName() override { return Name; }

        DX12Texture(std::string name, ID3D12Device* device, DX12CommandList* cmdList, int width, int height, int channels, uint8_t* data);
        void* GetPlatformResource() override { return Resource; }

    private:
        ID3D12Resource* Resource;
        std::string Name;
    };
}