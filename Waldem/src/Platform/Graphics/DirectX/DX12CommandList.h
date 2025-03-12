#pragma once
#include <d3d12.h>

#include "Waldem/Renderer/Model/Line.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/Renderer/RootSignature.h"

namespace Waldem
{
    class WALDEM_API DX12CommandList
    {
    public:
        DX12CommandList(ID3D12Device* device);
        ~DX12CommandList();
        bool CompileFromFile(const String& shaderName);

        void BeginInternal(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle);
        void EndInternal();

        void Draw(Model* model);
        void Draw(Mesh* mesh);
        void Dispatch(Point3 groupCount);
        void Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil, Vector3 clearColor);
        
        void SetPipeline(Pipeline* pipeline);
        void SetRootSignature(RootSignature* rootSignature);
        void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr);
        void SetDescriptorHeaps(uint32_t NumDescriptorHeaps, ID3D12DescriptorHeap* const* ppDescriptorHeaps);

        void* GetNativeCommandList() { return CommandList; }

        void* GetCommandAllocator() { return CommandAllocator; }

        void Execute(ID3D12CommandQueue* commandQueue);
        void WaitForCompletion();
        void ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier);
        void ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
        void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle);
        void ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle);
        void Reset();
        void SetConstants(uint32_t rootParamIndex, uint32_t numConstants, void* data, PipelineType pipelineType);
        void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox);
        void CopyResource(ID3D12Resource* dst, ID3D12Resource* src);
        void CopyRenderTarget(RenderTarget* dst, RenderTarget* src);
        void CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer);
        void UpdateBuffer(Buffer* buffer, void* data, uint32_t size);

        void UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);
        
        void Close();

    private:

        ID3D12Device* Device;
        ID3D12GraphicsCommandList* CommandList;
        ID3D12CommandAllocator* CommandAllocator;
        ID3D12Fence* Fence;
        HANDLE FenceEvent;
        UINT64 FenceValue;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentRenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentDepthStencilHandle;
        D3D12_VIEWPORT CurrentViewport;
        D3D12_RECT CurrentScissorRect;
        PixelShader* CurrentExecutableShader;
    };
}
