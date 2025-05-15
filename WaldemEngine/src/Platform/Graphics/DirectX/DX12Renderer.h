#pragma once
#include "Waldem/Renderer/Renderer.h"
#include <d3d12.h>
#include <dxgi1_6.h>

#include "DX12CommandList.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/RootSignature.h"
#include "Waldem/Renderer/Viewport.h"

namespace Waldem
{
    class DX12Renderer : public IRenderer 
    {
    public:
        ~DX12Renderer() override = default;
        void Initialize(CWindow* window) override;
        void InitializeUI() override;
        void DeinitializeUI() override;
        void ResizeEditorViewport(Vector2 size);
        void Draw(CModel* model) override;
        void Draw(CMesh* mesh) override;
        void DrawIndirect(CommandSignature* commandSignature, uint numCommands, Buffer* indirectBuffer) override;
        void SetIndexBuffer(Buffer* indexBuffer) override;
        void SetVertexBuffers(Buffer* vertexBuffer, uint32 numBuffers, uint32 startIndex = 0) override;
        void Signal() override;
        void Wait() override;
        Point3 GetNumThreadsPerGroup(ComputeShader* computeShader) override;
        void Compute(Point3 groupCount) override;
        void TraceRays(Pipeline* rayTracingPipeline, Point3 numRays) override;
        void Begin() override;
        void End() override;
        void Present() override;
        PixelShader* LoadPixelShader(const Path& shaderName, WString entryPoint) override;
        ComputeShader* LoadComputeShader(const Path& shaderName, WString entryPoint) override;
        RayTracingShader* LoadRayTracingShader(const Path& shaderName) override;
        void SetPipeline(Pipeline* pipeline) override;
        void SetRootSignature(RootSignature* rootSignature) override;
        void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr) override;
        void ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after) override;
        void ResourceBarrier(Buffer* buffer, ResourceStates before, ResourceStates after) override;
        Pipeline* CreateGraphicPipeline(const WString& name, RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout) override;
        Pipeline* CreateComputePipeline(const WString& name, RootSignature* rootSignature, ComputeShader* shader) override;
        Pipeline* CreateRayTracingPipeline(const WString& name, RootSignature* rootSignature, RayTracingShader* shader) override;
        RootSignature* CreateRootSignature(WArray<GraphicResource> resources) override;
        CommandSignature* CreateCommandSignature(RootSignature* rootSignature) override;
        Texture2D* CreateTexture(WString name, int width, int height, TextureFormat format, size_t dataSize, uint8_t* data = nullptr) override;
        Texture2D* CreateTexture(TextureDesc desc) override;
        RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format) override;
        SViewport* GetEditorViewport() override { return &EditorViewport; }
        SViewport* GetGameViewport() override { return &GameViewport; }
        SViewport* GetMainViewport() override { return &MainViewport; }
        AccelerationStructure* CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries) override;
        AccelerationStructure* CreateTLAS(WString name, WArray<RayTracingInstance>& instances) override;
        void UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries) override;
        void UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances) override;
        void CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT) override;
        void CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer) override;
        Buffer* CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride) override;
        void UpdateBuffer(Buffer* buffer, void* data, uint32_t size) override;
        void ClearRenderTarget(RenderTarget* rt) override;
        void ClearDepthStencil(RenderTarget* ds) override;
        void BeginUI() override;
        void EndUI() override;

    private:
        CWindow* CurrentWindow = nullptr;
        
        IDXGIFactory4* DxgiFactory = nullptr;
        ID3D12Device5* Device = nullptr;
        ID3D12CommandQueue* GraphicCommandQueue = nullptr;
        ID3D12CommandQueue* ComputeCommandQueue = nullptr;
        IDXGISwapChain* SwapChain = nullptr;

        std::pair<DX12CommandList*, bool> WorldCommandList;
        
        ID3D12Debug* DebugController = nullptr;
        ID3D12Debug1* DebugController1 = nullptr;
        ID3D12InfoQueue* InfoQueue = nullptr;
        ID3D12Fence* WaitFence = nullptr;
        ID3D12Fence* SecondaryWaitFence = nullptr;
        UINT64 SecondaryWaitFenceValue = 0;
        SViewport MainViewport = {};
        SViewport EditorViewport = {};
        SViewport GameViewport = {};
        bool ResizeTriggered = false;
        Point2 NewSize = {};

        //ImGui
        ID3D12DescriptorHeap* ImGuiHeap = nullptr;
    };
}
