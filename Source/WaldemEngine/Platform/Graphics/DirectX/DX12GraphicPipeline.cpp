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

    void FillBlendState(D3D12_BLEND_DESC& dx12State, BlendDesc& wdState)
    {
        dx12State.AlphaToCoverageEnable = wdState.AlphaToCoverageEnable;
        dx12State.IndependentBlendEnable = wdState.IndependentBlendEnable;

        for (int i = 0; i < 8; ++i)
        {
            auto& src = wdState.RenderTarget[i];
            auto& dst = dx12State.RenderTarget[i];

            dst.BlendEnable = src.BlendEnable;
            dst.SrcBlend = (D3D12_BLEND)src.SrcBlend;
            dst.DestBlend = (D3D12_BLEND)src.DestBlend;
            dst.BlendOp = (D3D12_BLEND_OP)src.BlendOp;
            dst.SrcBlendAlpha = (D3D12_BLEND)src.SrcBlendAlpha;
            dst.DestBlendAlpha = (D3D12_BLEND)src.DestBlendAlpha;
            dst.BlendOpAlpha = (D3D12_BLEND_OP)src.BlendOpAlpha;
            dst.RenderTargetWriteMask = src.RenderTargetWriteMask;
        }
    }
    
    DX12GraphicPipeline::DX12GraphicPipeline(const WString& name, ID3D12Device* device, ID3D12RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, TextureFormat depthFormat, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, BlendDesc blendDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::Graphics;
        Device = device;
        RootSignature = rootSignature;
        ShaderObject = shader;
        this->RTFormats = RTFormats;
        this->DepthFormat = depthFormat;
        RasterizerStateDesc = rasterizerDesc;
        DepthStencilStateDesc = depthStencilDesc;
        BlendStateDesc = blendDesc;
        PrimitiveTopology = primitiveTopologyType;
        InputLayoutDescs = inputLayout;
        if(!Reload())
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
    }

    DX12GraphicPipeline::~DX12GraphicPipeline()
    {
    }

    void DX12GraphicPipeline::Destroy()
    {
        if(NativePipeline != nullptr)
        {
            NativePipeline->Release();
            NativePipeline = nullptr;
        }
    }

    bool DX12GraphicPipeline::Reload()
    {
        IDxcBlob* vertexShaderBlob = (IDxcBlob*)ShaderObject->GetVS();
        IDxcBlob* pixelShaderBlob = (IDxcBlob*)ShaderObject->GetPS();
        if(vertexShaderBlob == nullptr || pixelShaderBlob == nullptr)
        {
            return false;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC newDesc = {};
        newDesc.pRootSignature = RootSignature;
        newDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        newDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        FillRasterizerState(newDesc.RasterizerState, RasterizerStateDesc);
        FillDepthStencilState(newDesc.DepthStencilState, DepthStencilStateDesc);
        FillBlendState(newDesc.BlendState, BlendStateDesc);
        newDesc.DSVFormat = (DXGI_FORMAT)DepthFormat;
        newDesc.SampleMask = UINT_MAX;
        newDesc.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)PrimitiveTopology;
        newDesc.NumRenderTargets = RTFormats.Num();

        for(uint32_t i = 0; i < RTFormats.Num(); ++i)
        {
            newDesc.RTVFormats[i] = (DXGI_FORMAT)RTFormats[i];
        }

        newDesc.SampleDesc.Count = 1;
        WArray<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
        for(auto& input : InputLayoutDescs)
        {
            inputElementDescs.Add({ input.SemanticName.C_Str(), input.SemanticIndex, (DXGI_FORMAT)input.Format, input.InputSlot, input.AlignedByteOffset, (D3D12_INPUT_CLASSIFICATION)input.InputSlotClass, input.InstanceDataStepRate });
        }

        newDesc.InputLayout = { inputElementDescs.GetData(), (UINT)inputElementDescs.Num() };

        ID3D12PipelineState* newPipeline = nullptr;
        HRESULT hr = Device->CreateGraphicsPipelineState(&newDesc, IID_PPV_ARGS(&newPipeline));
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
