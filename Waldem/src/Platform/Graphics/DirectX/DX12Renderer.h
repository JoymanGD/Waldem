#pragma once
#include "Waldem/Renderer/Renderer.h"
#include <d3d12.h>
#include <dxgi1_6.h>

#include "DX12CommandList.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/RootSignature.h"

namespace Waldem
{
    class DX12Renderer : public IRenderer
    {
    public:
        ~DX12Renderer() override = default;
        void Initialize(Window* window) override;
        void InitializeUI() override;
        void Draw(Model* model) override;
        void Draw(Mesh* mesh) override;
        void DrawLine(Line line) override;
        void DrawLines(WArray<Line> lines) override;
        void Wait() override;
        Point3 GetNumThreadsPerGroup(ComputeShader* computeShader) override;
        void Compute(Point3 groupCount) override;
        void Begin() override;
        void End() override;
        void Present() override;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetHandle() const { return CurrentRenderTargetHandle; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilHandle() const { return DSVHandle; }
        PixelShader* LoadPixelShader(String shaderName) override;
        ComputeShader* LoadComputeShader(String shaderName) override;
        void SetPipeline(Pipeline* pipeline) override;
        void SetRootSignature(RootSignature* rootSignature) override;
        void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr) override;
        void ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after) override;
        Pipeline* CreateGraphicPipeline(const String& name, WArray<TextureFormat> RTFormats, PrimitiveTopologyType primitiveTopologyType, RootSignature* rootSignature, PixelShader* shader) override;
        Pipeline* CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader) override;
        RootSignature* CreateRootSignature(WArray<Resource> resources) override;
        Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr) override;
        RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format) override;
        void CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT) override;
        VertexBuffer* CreateVertexBuffer(void* data, uint32_t size) override;
        IndexBuffer* CreateIndexBuffer(void* data, uint32_t size) override;
        void ClearRenderTarget(RenderTarget* rt) override;
        void ClearDepthStencil(RenderTarget* ds) override;
        void BeginUI() override;
        void EndUI() override;

    private:
        uint32_t FrameIndex = 0;

        Window* CurrentWindow;
        
        IDXGIFactory4* DxgiFactory = nullptr;
        ID3D12Device* Device = nullptr;
        ID3D12CommandQueue* GraphicCommandQueue = nullptr;
        ID3D12CommandQueue* ComputeCommandQueue = nullptr;
        IDXGISwapChain* SwapChain = nullptr;

        D3D12_VIEWPORT Viewport = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
        D3D12_RECT ScissorRect = { 0, 0, 800, 600 };

        std::pair<DX12CommandList*, bool> WorldCommandList;
        
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
        ID3D12Fence* WaitFence;
        UINT64 waitFenceValue = 0;

        //ImGui
        ID3D12DescriptorHeap* ImGuiHeap;
    };
}
