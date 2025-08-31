#pragma once
#include "Waldem/Renderer/Renderer.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include "DescriptorAllocator.h"
#include "DX12CommandList.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Viewport.h"

namespace Waldem
{
#define BINDLESS_MAX_DESCRIPTORS 10000
#define RTV_MAX_DESCRIPTORS 128

    struct RenderPassState
    {
        WArray<RenderTarget*> RenderTargets;
        RenderTarget* DepthStencil = nullptr;
        bool RenderTargetsDirty = false;
        bool DepthStencilDirty = false;
    };
    
    class DX12Renderer : public IRenderer
    {
    public:
        ~DX12Renderer() override = default;
        void Initialize(CWindow* window) override;
        void InitializeUI() override;
        void DeinitializeUI() override;
        void ResizeEditorViewport(Vector2 size);
        void Draw(CMesh* mesh) override;
        void DrawIndexedInstanced(uint indicesCount, uint instancesCount, uint startIndex, int baseVertex, uint startInstance) override;
        void DrawIndirect(uint numCommands, Buffer* indirectBuffer) override;
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
        void PushConstants(void* data, size_t size) override;
        void BindRenderTargets(WArray<RenderTarget*> renderTargets = {}) override;
        void BindDepthStencil(RenderTarget* depthStencil = nullptr) override;
        void SetViewport(SViewport& viewport) override;
        void ResourceBarrier(GraphicResource* resource, ResourceStates before, ResourceStates after) override;
        ResourceStates ResourceBarrier(GraphicResource* resource, ResourceStates after) override;
        void UAVBarrier(GraphicResource* resource) override;
        Pipeline* CreateGraphicPipeline(const WString& name, PixelShader* shader, WArray<TextureFormat> RTFormats, TextureFormat depthFormat, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout) override;
        Pipeline* CreateComputePipeline(const WString& name, ComputeShader* shader) override;
        Pipeline* CreateRayTracingPipeline(const WString& name, RayTracingShader* shader) override;
        Texture2D* CreateTexture2D(WString name, int width, int height, TextureFormat format, uint8_t* data = nullptr) override;
        Texture3D* CreateTexture3D(WString name, int width, int height, int depth, TextureFormat format, uint8_t* data = nullptr) override;
        RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format) override;
        void InitializeRenderTarget(WString name, int width, int height, TextureFormat format, RenderTarget*& renderTarget) override;
        Buffer* CreateBuffer(WString name, BufferType type, uint32_t size, uint32_t stride, void* data, size_t dataSize) override;
        void InitializeBuffer(WString name, BufferType type, uint32_t size, uint32_t stride, Buffer*& buffer, void* data, size_t dataSize) override;
        AccelerationStructure* CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries) override;
        AccelerationStructure* CreateTLAS(WString name, Buffer* instanceBuffer, uint numInstances) override;
        void InitializeTLAS(WString name, Buffer* instanceBuffer, uint numInstances, AccelerationStructure*& tlas) override;
        void BuildTLAS(Buffer* instanceBuffer, uint numInstances, AccelerationStructure*& tlas) override;
        void UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries) override;
        void UpdateTLAS(AccelerationStructure* TLAS, Buffer* instanceBuffer, uint numInstances) override;
        SViewport* GetEditorViewport() override { return &EditorViewport; }
        SViewport* GetGameViewport() override { return &GameViewport; }
        SViewport* GetMainViewport() override { return &MainViewport; }
        void CopyResource(GraphicResource* dstResource, GraphicResource* srcResource) override;
        void CopyBufferRegion(GraphicResource* dstResource, size_t dstOffset, GraphicResource* srcResource, size_t srcOffset, size_t size) override;
        void UploadBuffer(Buffer* buffer, void* data, uint32_t size, uint offset = 0) override;
        void ClearBuffer(Buffer* buffer, uint32_t size, uint offset = 0) override;
        void DownloadBuffer(Buffer* buffer, void* data, size_t size) override;
        void ClearRenderTarget(RenderTarget* rt) override;
        void ClearDepthStencil(RenderTarget* ds) override;
        void BeginUI() override;
        void EndUI() override;
        void Destroy(GraphicResource* resource) override; 
        void DestroyImmediate(GraphicResource* resource) override; 
        void* GetPlatformResource(GraphicResource* resource) override;

    private:
        void InitializeBindless();
        void CreateGeneralRootSignature();
        RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format, ID3D12DescriptorHeap* externalHeap, uint slot);
        RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format, ID3D12Resource* resource);
        void SetRenderTargets();

        CWindow* CurrentWindow = nullptr;

        PipelineType CurrentPipelineType = PipelineType::Graphics;
        
        IDXGIFactory4* DxgiFactory = nullptr;
        ID3D12Device5* Device = nullptr;
        ID3D12CommandQueue* GraphicCommandQueue = nullptr;
        ID3D12CommandQueue* ComputeCommandQueue = nullptr;
        IDXGISwapChain* SwapChain = nullptr;
        ID3D12DescriptorHeap* RTVHeap = nullptr;
        ID3D12DescriptorHeap* DSVHeap = nullptr;
        DescriptorAllocator RenderTargetAllocator = DescriptorAllocator(128); //0..128

        RenderPassState CurrentRenderPassState;
        RenderPassState DefaultRenderPassState;

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

        WArray<ID3D12Resource*> ResourcesToDestroy;

        //ImGui
        ID3D12DescriptorHeap* ImGuiHeap = nullptr;

        //bindless
        WMap<GraphicResource*, ID3D12Resource*> ResourceMap;
        DescriptorAllocator GeneralAllocator = DescriptorAllocator(BINDLESS_MAX_DESCRIPTORS);
        ID3D12DescriptorHeap* GeneralResourcesHeap = nullptr;
        ID3D12DescriptorHeap* GeneralSamplerHeap = nullptr;
        ID3D12RootSignature* GeneralRootSignature = nullptr;
        ID3D12CommandSignature* GeneralCommandSignature = nullptr;
    };
}
