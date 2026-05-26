#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12ComputePipeline : public Pipeline
    {
    public:
        DX12ComputePipeline(const WString& name, ID3D12Device* device, ID3D12RootSignature* rootSignature, ComputeShader* shader);
        ~DX12ComputePipeline() override;
        bool Reload() override;
        void* GetNativeObject() const override { return NativePipeline; }
        D3D12_COMPUTE_PIPELINE_STATE_DESC* GetDesc() { return &PsoDesc; }
        void Destroy() override;

    private:
        ID3D12Device* Device = nullptr;
        ID3D12RootSignature* RootSignature = nullptr;
        ComputeShader* ShaderObject = nullptr;
        ID3D12PipelineState* NativePipeline = nullptr;
        D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc;
    };
}
