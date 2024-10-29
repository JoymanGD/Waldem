#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12PixelShader : public PixelShader
    {
    public:
        DX12PixelShader(ID3D12Device* device, const std::string& shaderName);
        ~DX12PixelShader() override;

        ID3D12PipelineState* GetPipeline() const { return pipelineState; }

    private:
        bool CompileFromFile(const std::string& filepath);
        
        ID3DBlob* vertexShader;
        ID3DBlob* pixelShader;
        ID3DBlob* errorBlob;
        ID3D12PipelineState* pipelineState;
        ID3D12RootSignature* rootSignature;
    };
}
