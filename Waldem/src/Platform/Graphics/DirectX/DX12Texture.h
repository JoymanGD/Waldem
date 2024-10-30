#pragma once
#include "Waldem/Renderer/Texture.h"
#include <d3d12.h>

namespace Waldem
{
    class WALDEM_API DX12Texture : public Texture2D
    {
    public:
        std::string GetName() override { return Name; }

        DX12Texture(ID3D12Device* device, std::string name, int width, int height, int channels, uint8_t* data);
        
    private:
        ID3D12Resource* Resource;
        std::string Name;
    };
}