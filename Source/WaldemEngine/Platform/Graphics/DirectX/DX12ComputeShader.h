#pragma once
#include <d3dcommon.h>
#include <dxcapi.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12ComputeShader : public ComputeShader
    {
    public:
        DX12ComputeShader(const Path& name, const WString& entryPoint);
        ~DX12ComputeShader() override;

    private:
        bool CompileFromFile(const Path& filepath, const WString& entryPoint) override;

    public:
        void* GetPlatformData() override { return ShaderBlob; }
        void Destroy() override { if (ShaderBlob) ShaderBlob->Release(); if (DxcUtils) DxcUtils->Release(); if (DxcCompiler) DxcCompiler->Release(); if (DxcIncludeHandler) DxcIncludeHandler->Release(); }

    private:
        IDxcBlob* ShaderBlob;
        IDxcBlobUtf8* ErrorBlob;
        IDxcUtils* DxcUtils;
        IDxcCompiler3* DxcCompiler;
        IDxcIncludeHandler* DxcIncludeHandler;
        IDxcBlobEncoding* Source;
    };
}
