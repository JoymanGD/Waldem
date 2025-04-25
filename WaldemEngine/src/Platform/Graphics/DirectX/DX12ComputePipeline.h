#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/RootSignature.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12ComputePipeline : public Pipeline
    {
    public:
        DX12ComputePipeline(const WString& name, ID3D12Device* device, RootSignature* rootSignature, ComputeShader* shader);
        ~DX12ComputePipeline() override;
        void* GetNativeObject() const override { return NativePipeline; }
        D3D12_COMPUTE_PIPELINE_STATE_DESC* GetDesc() { return &PsoDesc; }
        void Destroy() override { NativePipeline->Release(); }

    private:
        ID3D12PipelineState* NativePipeline;
        D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc;
    };
}
