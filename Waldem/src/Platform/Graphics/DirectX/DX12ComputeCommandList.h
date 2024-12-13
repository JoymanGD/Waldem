#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    class WALDEM_API DX12ComputeCommandList
    {
    public:
        DX12ComputeCommandList(ID3D12Device* device);
        ~DX12ComputeCommandList();

        void* GetNativeCommandList() const { return CommandList; }

        void* GetCommandAllocator() const { return CommandAllocator; }

        void Dispatch(Point3 groupCount);
        void Execute(ID3D12CommandQueue* commandQueue);
        void WaitForCompletion();
        void ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier);
        void Reset();
        void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox);
        
        void UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);
        void Close();

    private:
        ID3D12Device* Device;
        ID3D12Resource* CurrentRenderTarget;
        ID3D12GraphicsCommandList* CommandList;
        ID3D12CommandAllocator* CommandAllocator;
        ID3D12Fence* Fence;
        HANDLE FenceEvent;
        UINT64 FenceValue;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentRenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentDepthStencilHandle;
        D3D12_VIEWPORT CurrentViewport;
        D3D12_RECT CurrentScissorRect;
    };
}
