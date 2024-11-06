#pragma once
#include <d3d12.h>

namespace Waldem
{
    class WALDEM_API ResourceManager
    {
    public:
        ResourceManager() = default;
        
        void CreateBuffer();
        void CreateTexture();
        void DestroyResource();

        private:
        std::vector<ID3D12Resource*> resources;
    };
}
