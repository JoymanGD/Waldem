#pragma once
#include "Waldem/Renderer/Renderer.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include "DX12CommandList.h"

namespace Waldem
{
    class DX12Renderer : public IRenderer
    {
    public:
        ~DX12Renderer() override = default;
        void Initialize(Window* window) override;
        void Draw(Model* model, PixelShader* pixelShader) override;
        void Draw(Mesh* mesh, PixelShader* pixelShader) override;
        void Begin() override;
        void End() override;
        void Present() override;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetHandle() const { return CurrentRenderTargetHandle; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilHandle() const { return DSVHandle; }
        PixelShader* LoadPixelShader(String shaderName, std::vector<Resource> resources, RenderTarget* renderTarget = nullptr) override;
        Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr) override;
        RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format) override;
        VertexBuffer* CreateVertexBuffer(void* data, uint32_t size) override;
        IndexBuffer* CreateIndexBuffer(void* data, uint32_t size) override;

    private:
        uint32_t FrameIndex = 0;
        
        IDXGIFactory4* DxgiFactory = nullptr;
        ID3D12Device* Device = nullptr;
        ID3D12CommandQueue* CommandQueue = nullptr;
        IDXGISwapChain* SwapChain = nullptr;

        D3D12_VIEWPORT Viewport = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
        D3D12_RECT ScissorRect = { 0, 0, 800, 600 };

        std::pair<DX12CommandList*, bool> WorldCommandList;
        std::pair<DX12CommandList*, bool> UICommandList;
        
        ID3D12DescriptorHeap* RTVHeap;
        ID3D12DescriptorHeap* DSVHeap;
        uint32_t RTVDescriptorSize;
        ID3D12Resource* RenderTargets[SWAPCHAIN_SIZE];
        ID3D12Resource* DepthStencilBuffer;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentRenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle;
        ID3D12Debug* DebugController;
        ID3D12Debug1* DebugController1;
        ID3D12InfoQueue* InfoQueue;
    };
}
