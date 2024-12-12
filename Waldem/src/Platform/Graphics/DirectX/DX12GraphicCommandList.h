#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    class WALDEM_API DX12GraphicCommandList
    {
    public:
        DX12GraphicCommandList(ID3D12Device* device);
        ~DX12GraphicCommandList();

        void BeginInternal(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle);
        void EndInternal();

        void BeginDraw(PixelShader* shader);
        void EndDraw(PixelShader* shader);

        void Draw(Model* model);
        void Draw(Mesh* mesh);
        void Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil, Vector3 clearColor);

        void* GetNativeCommandList() const { return CommandList; }

        void* GetCommandAllocator() const { return CommandAllocator; }

        void Execute(ID3D12CommandQueue* commandQueue);
        void WaitForCompletion();
        void ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier);
        void ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
        void Reset();
        void SetConstants(uint32_t slot, uint32_t numConstants, void* data);
        void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox);
        void CopyResource(ID3D12Resource* dst, ID3D12Resource* src);
        
        void UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

    private:
        
        void Close();

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
        PixelShader* CurrentExecutableShader;
    };
}
