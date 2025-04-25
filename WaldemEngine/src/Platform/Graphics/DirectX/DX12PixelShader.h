#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include <dxcapi.h>

#include "DX12CommandList.h"
#include "DX12Resource.h"
#include "DX12Shader.h"
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Renderer/GraphicResource.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12PixelShader : public PixelShader
    {
    public:
        DX12PixelShader(const Path& name, const WString& entryPoint);
        ~DX12PixelShader() override;

    private:
        bool CompileFromFile(const Path& filepath, const WString& entryPoint) override;

    public:
        void* GetVS() override { return VertexShaderBlob; }
        void* GetPS() override { return PixelShaderBlob; }
        void Destroy() override { if (VertexShaderBlob) VertexShaderBlob->Release(); if (PixelShaderBlob) PixelShaderBlob->Release(); if (DxcUtils) DxcUtils->Release(); if (DxcCompiler) DxcCompiler->Release(); if (DxcIncludeHandler) DxcIncludeHandler->Release(); }

    private:
        IDxcBlob* VertexShaderBlob;
        IDxcBlob* PixelShaderBlob;
        IDxcBlobUtf8* ErrorBlob;
        IDxcUtils* DxcUtils;
        IDxcCompiler3* DxcCompiler;
        IDxcIncludeHandler* DxcIncludeHandler;
        IDxcBlobEncoding* Source;
    };
}
