#pragma once
#include <d3d12.h>

namespace Waldem
{
    class WALDEM_API CommandList
    {
    public:
        CommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator);
        
        void BeginRecording();  // Start recording commands
        void EndRecording();    // Finish and close command list
        void Execute();         // Submit to GPU for execution

    private:
        ID3D12GraphicsCommandList* commandList;
        ID3D12CommandAllocator* commandAllocator;
    };
}
