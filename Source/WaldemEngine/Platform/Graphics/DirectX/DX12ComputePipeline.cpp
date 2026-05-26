#include "wdpch.h"
#include "DX12ComputePipeline.h"

#include <dxcapi.h>

#include "DX12Helper.h"

namespace Waldem
{
    DX12ComputePipeline::DX12ComputePipeline(const WString& name, ID3D12Device* device, ID3D12RootSignature* rootSignature, ComputeShader* shader) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::Compute;
        Device = device;
        RootSignature = rootSignature;
        ShaderObject = shader;
        if(!Reload())
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
    }

    DX12ComputePipeline::~DX12ComputePipeline()
    {
    }

    void DX12ComputePipeline::Destroy()
    {
        if(NativePipeline != nullptr)
        {
            NativePipeline->Release();
            NativePipeline = nullptr;
        }
    }

    bool DX12ComputePipeline::Reload()
    {
        IDxcBlob* computeShaderBlob = (IDxcBlob*)ShaderObject->GetPlatformData();
        if(computeShaderBlob == nullptr)
        {
            return false;
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC newDesc = {};
        newDesc.pRootSignature = RootSignature;
        newDesc.CS = { computeShaderBlob->GetBufferPointer(), computeShaderBlob->GetBufferSize() };

        ID3D12PipelineState* newPipeline = nullptr;
        HRESULT hr = Device->CreateComputePipelineState(&newDesc, IID_PPV_ARGS(&newPipeline));
        if(FAILED(hr))
        {
            return false;
        }

        if(NativePipeline != nullptr)
        {
            NativePipeline->Release();
        }

        NativePipeline = newPipeline;
        PsoDesc = newDesc;
        NativePipeline->SetName(DX12Helper::WFromMB(Name));
        return true;
    }
}
