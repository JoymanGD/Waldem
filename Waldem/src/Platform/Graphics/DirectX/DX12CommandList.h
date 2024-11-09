#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12CommandList
    {
    public:
        DX12CommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
        ~DX12CommandList();

        void Begin(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle);
        void End();

        void AddDrawCommand(Mesh* mesh, PixelShader* shader, uint32_t meshID);
        void Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil, Vector3 clearColor);

        void* GetNativeCommandList() const { return CommandList; }

        void* GetCommandAllocator() const { return CommandAllocator; }

        void Execute(void* commandQueue);
        void WaitForCompletion();
        void ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier);
        void Reset();
        void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox);
        
        void UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

    private:
        
        void Close();

        ID3D12Device* Device;
        ID3D12GraphicsCommandList* CommandList;
        ID3D12CommandAllocator* CommandAllocator;
        ID3D12Fence* Fence;
        HANDLE FenceEvent;
        UINT64 FenceValue;
    };
}
