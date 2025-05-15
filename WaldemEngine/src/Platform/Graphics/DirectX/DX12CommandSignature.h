#pragma once
#include <d3d12.h>

#include "DX12CommandList.h"
#include "Waldem/Renderer/CommandSignature.h"

namespace Waldem
{
    
    class WALDEM_API DX12CommandSignature : public CommandSignature
    {
    public:
        DX12CommandSignature(ID3D12Device* device, RootSignature* rootSignature);
        ~DX12CommandSignature() override {}
        void* GetNativeObject() const override { return NativeCommandSignature; }
        void Destroy() override;

    private:
        ID3D12CommandSignature* NativeCommandSignature = nullptr;
    };
}
