#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/RootSignature.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12Pipeline : public Pipeline
    {
    public:
        DX12Pipeline(const String& name, WArray<TextureFormat> RTFormats, PrimitiveTopologyType primitiveTopologyType, ID3D12Device* device, RootSignature* rootSignature, PixelShader* shader);
        ~DX12Pipeline() override;
        void* GetNativeObject() const override { return NativePipeline; }
        D3D12_GRAPHICS_PIPELINE_STATE_DESC* GetDesc() { return &PsoDesc; }

    private:
        ID3D12PipelineState* NativePipeline;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc;
    };
}