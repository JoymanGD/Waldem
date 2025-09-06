#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12GraphicPipeline : public Pipeline
    {
    public:
        DX12GraphicPipeline(const WString& name, ID3D12Device* device, ID3D12RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, TextureFormat depthFormat, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, BlendDesc blendDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout);
        ~DX12GraphicPipeline() override;
        void* GetNativeObject() const override { return NativePipeline; }
        D3D12_GRAPHICS_PIPELINE_STATE_DESC* GetDesc() { return &PsoDesc; }
        void Destroy() override { NativePipeline->Release(); }

    private:
        ID3D12PipelineState* NativePipeline;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc;
    };
}
