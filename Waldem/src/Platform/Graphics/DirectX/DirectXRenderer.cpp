#include "wdpch.h"
#include "DirectXRenderer.h"

namespace Waldem
{
    void DirectXRenderer::Initialize(Window* window)
    {
        CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory));

        D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue));

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 2;  // Double buffering
        swapChainDesc.BufferDesc.Width = window->GetWidth();
        swapChainDesc.BufferDesc.Height = window->GetHeight();
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = window->GetWindowsHandle();
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        DxgiFactory->CreateSwapChain(CommandQueue, &swapChainDesc, &SwapChain);
    }

    void DirectXRenderer::Clear(Vector4 clearColor)
    {
    }

    void DirectXRenderer::Render(uint32_t indexCount)
    {
    }
}
