#pragma once
#include <d3d12.h>

#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    class WALDEM_API DX12CommandList
    {
    public:
        DX12CommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
        ~DX12CommandList();

        void Begin(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTarget);
        void End();

        void AddDrawCommand(Mesh* mesh, PixelShader* shader);
        void Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, Vector3 clearColor);

        void* GetNativeCommandList() const { return commandList; }

        void* GetCommandAllocator() const { return commandAllocator; }

        void Execute(void* commandQueue);
        void WaitForCompletion();
        void ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier);
        void Reset();

    private:
        
        void Close();

        ID3D12GraphicsCommandList* commandList;
        ID3D12CommandAllocator* commandAllocator;
        ID3D12Fence* fence;
        HANDLE fenceEvent;
        UINT64 fenceValue;
    };
}
