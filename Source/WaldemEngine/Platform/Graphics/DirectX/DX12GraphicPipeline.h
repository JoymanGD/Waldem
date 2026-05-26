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
        bool Reload() override;
        void* GetNativeObject() const override { return NativePipeline; }
        D3D12_GRAPHICS_PIPELINE_STATE_DESC* GetDesc() { return &PsoDesc; }
        void Destroy() override;

    private:
        ID3D12Device* Device = nullptr;
        ID3D12RootSignature* RootSignature = nullptr;
        PixelShader* ShaderObject = nullptr;
        WArray<TextureFormat> RTFormats;
        TextureFormat DepthFormat = TextureFormat::D32_FLOAT;
        RasterizerDesc RasterizerStateDesc = DEFAULT_RASTERIZER_DESC;
        DepthStencilDesc DepthStencilStateDesc = DEFAULT_DEPTH_STENCIL_DESC;
        BlendDesc BlendStateDesc = DEFAULT_BLEND_DESC;
        PrimitiveTopologyType PrimitiveTopology = WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        WArray<InputLayoutDesc> InputLayoutDescs;
        ID3D12PipelineState* NativePipeline = nullptr;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc;
    };
}
