#include "wdpch.h"
#include "DX12ComputePipeline.h"

#include <dxcapi.h>

#include "DX12Helper.h"

namespace Waldem
{
    DX12ComputePipeline::DX12ComputePipeline(const WString& name, ID3D12Device* device, ID3D12RootSignature* rootSignature, ComputeShader* shader) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::Compute;
        
        IDxcBlob* ComputeShaderBlob = (IDxcBlob*)shader->GetPlatformData();
        
        PsoDesc = {};
        PsoDesc.pRootSignature = rootSignature;
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
