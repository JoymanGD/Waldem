#include "wdpch.h"
#include "DX12ComputePipeline.h"

namespace Waldem
{
    DX12ComputePipeline::DX12ComputePipeline(const String& name, ID3D12Device* device, RootSignature* rootSignature, ComputeShader* shader) : Pipeline(name)
    {
        rootSignature->CurrentPipelineType = PipelineType::Compute;
        
        ID3DBlob* ComputeShaderBlob = (ID3DBlob*)shader->GetPlatformData();
        
        PsoDesc = {};
        PsoDesc.pRootSignature = (ID3D12RootSignature*)rootSignature->GetNativeObject();
        PsoDesc.CS = { ComputeShaderBlob->GetBufferPointer(), ComputeShaderBlob->GetBufferSize() };
        
        HRESULT hr = device->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&NativePipeline));

        if(FAILED(hr))
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
    }

    DX12ComputePipeline::~DX12ComputePipeline()
    {
    }
}
