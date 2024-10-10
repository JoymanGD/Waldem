#pragma once
#include "Waldem/Renderer/Renderer.h"
#include <d3d12.h>
#include <dxgi1_6.h>

namespace Waldem
{
    class DirectXRenderer : public IRenderer
    {
    public:
        ~DirectXRenderer() override = default;
        void Initialize(Window* window) override;
        void Clear(Vector4 clearColor) override;
        void Render(uint32_t indexCount) override;
    private:
        IDXGIFactory4* DxgiFactory = nullptr;
        ID3D12Device* Device = nullptr;
        ID3D12CommandQueue* CommandQueue = nullptr;
        IDXGISwapChain* SwapChain = nullptr;
    };
}