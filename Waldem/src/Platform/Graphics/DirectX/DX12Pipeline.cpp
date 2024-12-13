#include "wdpch.h"
#include "DX12Pipeline.h"

namespace Waldem
{
    DX12Pipeline::DX12Pipeline(const String& name, WArray<TextureFormat> RTFormats, PrimitiveTopologyType primitiveTopologyType, ID3D12Device* device, RootSignature* rootSignature, PixelShader* shader) : Pipeline(name)
    {
        // Pipeline state
        ID3DBlob* VertexShaderBlob = (ID3DBlob*)shader->GetVS();
        ID3DBlob* PixelShaderBlob = (ID3DBlob*)shader->GetPS();
        
        PsoDesc = {};
        PsoDesc.pRootSignature = (ID3D12RootSignature*)rootSignature->GetNativeObject();
        PsoDesc.VS = { VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize() };
        PsoDesc.PS = { PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize() };
        PsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        PsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        PsoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        PsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        PsoDesc.DepthStencilState.StencilEnable = FALSE;
        PsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        PsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        PsoDesc.DepthStencilState.DepthEnable = TRUE;
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
        
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "MESH_ID", 0, DXGI_FORMAT_R16_UINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        
        PsoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        HRESULT hr = device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&NativePipeline));

        if(FAILED(hr))
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
    }

    DX12Pipeline::~DX12Pipeline()
    {
    }
}
