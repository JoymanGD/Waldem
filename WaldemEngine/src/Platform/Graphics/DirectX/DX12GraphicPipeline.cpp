#include "wdpch.h"
#include "DX12GraphicPipeline.h"

#include <dxcapi.h>

#include "DX12Helper.h"

namespace Waldem
{
    void FillRasterizerState(D3D12_RASTERIZER_DESC& dx12State, RasterizerDesc& wdState)
    {
        dx12State.ConservativeRaster = (D3D12_CONSERVATIVE_RASTERIZATION_MODE)wdState.ConservativeRaster;
        dx12State.CullMode = (D3D12_CULL_MODE)wdState.CullMode;
        dx12State.DepthBias = wdState.DepthBias;
        dx12State.DepthBiasClamp = wdState.DepthBiasClamp;
        dx12State.DepthClipEnable = wdState.DepthClipEnable;
        dx12State.FillMode = (D3D12_FILL_MODE)wdState.FillMode;
        dx12State.ForcedSampleCount = wdState.ForcedSampleCount;
        dx12State.FrontCounterClockwise = wdState.FrontCounterClockwise;
        dx12State.MultisampleEnable = wdState.MultisampleEnable;
        dx12State.SlopeScaledDepthBias = wdState.SlopeScaledDepthBias;
    }

    void FillDepthStencilState(D3D12_DEPTH_STENCIL_DESC& dx12State, DepthStencilDesc& wdState)
    {
        dx12State.DepthEnable = wdState.DepthEnable;
        dx12State.DepthWriteMask = (D3D12_DEPTH_WRITE_MASK)wdState.DepthWriteMask;
        dx12State.DepthFunc = (D3D12_COMPARISON_FUNC)wdState.DepthFunc;
        dx12State.StencilEnable = wdState.StencilEnable;
        dx12State.StencilReadMask = wdState.StencilReadMask;
        dx12State.StencilWriteMask = wdState.StencilWriteMask;
        dx12State.FrontFace.StencilFailOp = (D3D12_STENCIL_OP)wdState.FrontFace.StencilFailOp;
        dx12State.FrontFace.StencilDepthFailOp = (D3D12_STENCIL_OP)wdState.FrontFace.StencilDepthFailOp;
        dx12State.FrontFace.StencilPassOp = (D3D12_STENCIL_OP)wdState.FrontFace.StencilPassOp;
        dx12State.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC)wdState.FrontFace.StencilFunc;
        dx12State.BackFace.StencilFailOp = (D3D12_STENCIL_OP)wdState.BackFace.StencilFailOp;
        dx12State.BackFace.StencilDepthFailOp = (D3D12_STENCIL_OP)wdState.BackFace.StencilDepthFailOp;
        dx12State.BackFace.StencilPassOp = (D3D12_STENCIL_OP)wdState.BackFace.StencilPassOp;
        dx12State.BackFace.StencilFunc = (D3D12_COMPARISON_FUNC)wdState.BackFace.StencilFunc;
    }
    
    DX12GraphicPipeline::DX12GraphicPipeline(const WString& name, ID3D12Device* device, ID3D12RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::Graphics;
        
        IDxcBlob* VertexShaderBlob = (IDxcBlob*)shader->GetVS();
        IDxcBlob* PixelShaderBlob = (IDxcBlob*)shader->GetPS();
        
        PsoDesc = {};
        PsoDesc.pRootSignature = rootSignature;
        PsoDesc.VS = { VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize() };
        PsoDesc.PS = { PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize() };
        FillRasterizerState(PsoDesc.RasterizerState, rasterizerDesc);
        FillDepthStencilState(PsoDesc.DepthStencilState, depthStencilDesc);
        PsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        PsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        PsoDesc.SampleMask = UINT_MAX;
        PsoDesc.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)primitiveTopologyType;
        PsoDesc.NumRenderTargets = RTFormats.Num();
        // PsoDesc.RTVFormats[0] = renderTarget ? (DXGI_FORMAT)renderTarget->GetFormat() : DXGI_FORMAT_R8G8B8A8_UNORM;

        for (uint32_t i = 0; i < RTFormats.Num(); ++i)
        {
            PsoDesc.RTVFormats[i] = (DXGI_FORMAT)RTFormats[i];
        }
        
        PsoDesc.SampleDesc.Count = 1;
        WArray<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

        for (auto& input : inputLayout)
        {
            inputElementDescs.Add({ input.SemanticName.C_Str(), input.SemanticIndex, (DXGI_FORMAT)input.Format, input.InputSlot, input.AlignedByteOffset, (D3D12_INPUT_CLASSIFICATION)input.InputSlotClass, input.InstanceDataStepRate });
        }
        
        PsoDesc.InputLayout = { inputElementDescs.GetData(), (UINT)inputElementDescs.Num() };
        HRESULT hr = device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&NativePipeline));

        if(FAILED(hr))
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
        
        NativePipeline->SetName(DX12Helper::WFromMB(name));
    }

    DX12GraphicPipeline::~DX12GraphicPipeline()
    {
    }
}
