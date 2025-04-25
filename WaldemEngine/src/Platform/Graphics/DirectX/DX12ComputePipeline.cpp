#include "wdpch.h"
#include "DX12ComputePipeline.h"

#include <dxcapi.h>

#include "DX12Helper.h"

namespace Waldem
{
    DX12ComputePipeline::DX12ComputePipeline(const WString& name, ID3D12Device* device, RootSignature* rootSignature, ComputeShader* shader) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::Compute;
        
        rootSignature->CurrentPipelineType = PipelineType::Compute;
        
        IDxcBlob* ComputeShaderBlob = (IDxcBlob*)shader->GetPlatformData();
        
        PsoDesc = {};
        PsoDesc.pRootSignature = (ID3D12RootSignature*)rootSignature->GetNativeObject();
        PsoDesc.CS = { ComputeShaderBlob->GetBufferPointer(), ComputeShaderBlob->GetBufferSize() };
        
        HRESULT hr = device->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&NativePipeline));
        NativePipeline->SetName(DX12Helper::WFromMB(name));

        if(FAILED(hr))
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
    }

    DX12ComputePipeline::~DX12ComputePipeline()
    {
    }
}
