#include "wdpch.h"
#include "DX12CommandSignature.h"

#include "DX12RootSignature.h"

namespace Waldem
{
    DX12CommandSignature::DX12CommandSignature(ID3D12Device* device, RootSignature* rootSignature)
    {
        D3D12_INDIRECT_ARGUMENT_DESC args[2] = {};
        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        args[0].Constant.RootParameterIndex = 0;
        args[0].Constant.Num32BitValuesToSet = 1;
        args[0].Constant.DestOffsetIn32BitValues = 0;
        
        args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC commandSigDesc = {};
        commandSigDesc.ByteStride = sizeof(uint) + sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        commandSigDesc.NumArgumentDescs = 2;
        commandSigDesc.pArgumentDescs = args;

        ID3D12RootSignature* nativeRootSignature = (ID3D12RootSignature*)static_cast<DX12RootSignature*>(rootSignature)->GetNativeObject();

        device->CreateCommandSignature(&commandSigDesc, nativeRootSignature, IID_PPV_ARGS(&NativeCommandSignature));
    }

    void DX12CommandSignature::Destroy()
    {
        NativeCommandSignature->Release();
    }
}
