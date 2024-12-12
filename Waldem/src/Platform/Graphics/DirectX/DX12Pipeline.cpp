#include "wdpch.h"
#include "DX12Pipeline.h"

namespace Waldem
{
    DX12Pipeline::DX12Pipeline(const String& name, WD_TEXTURE_FORMAT RTFormats[8], ID3D12Device* device, RootSignature* rootSignature, PixelShader* shader) : Pipeline(name, RTFormats)
    {
        // Pipeline state
        ID3DBlob* VertexShaderBlob = (ID3DBlob*)shader->GetVS();
        ID3DBlob* PixelShaderBlob = (ID3DBlob*)shader->GetPS();
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = (ID3D12RootSignature*)rootSignature->GetNativeObject();
        psoDesc.VS = { VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize() };
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        // psoDesc.RTVFormats[0] = renderTarget ? (DXGI_FORMAT)renderTarget->GetFormat() : DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.RTVFormats[0] = (DXGI_FORMAT)RTFormats[0];
        psoDesc.RTVFormats[1] = (DXGI_FORMAT)RTFormats[1];
        psoDesc.RTVFormats[2] = (DXGI_FORMAT)RTFormats[2];
        psoDesc.RTVFormats[3] = (DXGI_FORMAT)RTFormats[3];
        psoDesc.RTVFormats[4] = (DXGI_FORMAT)RTFormats[4];
        psoDesc.RTVFormats[5] = (DXGI_FORMAT)RTFormats[5];
        psoDesc.RTVFormats[6] = (DXGI_FORMAT)RTFormats[6];
        psoDesc.RTVFormats[7] = (DXGI_FORMAT)RTFormats[7];
        
        psoDesc.SampleDesc.Count = 1;
        
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "MESH_ID", 0, DXGI_FORMAT_R16_UINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&NativePipeline));

        if(FAILED(hr))
        {
            throw std::runtime_error("Failed to create pipeline state!");
        }
    }
}
