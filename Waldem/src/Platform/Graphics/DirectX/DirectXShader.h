#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DirectXPixelShader : public PixelShader
    {
    public:
        DirectXPixelShader(const std::string& shaderName);
        ~DirectXPixelShader() override;
        void Bind() const override;
        void Unbind() const override;

    private:
        bool CompileFromFile(const std::string& filepath);
        bool CreatePipelineState(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
        
        ID3DBlob* vertexShader;
        ID3DBlob* pixelShader;
        ID3DBlob* errorBlob;
        ID3D12PipelineState* pipelineState;
    };
}
