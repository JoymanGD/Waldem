#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "DX12CommandList.h"
#include "DX12Resource.h"
#include "DX12Shader.h"
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12PixelShader : public PixelShader
    {
    public:
        DX12PixelShader(const String& name);
        ~DX12PixelShader() override;

    private:
        bool CompileFromFile(const String& filepath) override;

    public:
        void* GetVS() override { return VertexShaderBlob; }
        void* GetPS() override { return PixelShaderBlob; }

    private:
        ID3DBlob* VertexShaderBlob;
        ID3DBlob* PixelShaderBlob;
        ID3DBlob* ErrorBlob;
    };
}
