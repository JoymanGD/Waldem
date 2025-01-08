#pragma once
#include <d3dcommon.h>
#include <dxcapi.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12ComputeShader : public ComputeShader
    {
    public:
        DX12ComputeShader(const String& name, const String& entryPoint);
        ~DX12ComputeShader() override;

    private:
        bool CompileFromFile(const String& filepath, const String& entryPoint) override;

    public:
        void* GetPlatformData() override { return ShaderBlob; }

    private:
        IDxcBlob* ShaderBlob;
        IDxcBlobUtf8* ErrorBlob;
        IDxcUtils* DxcUtils;
        IDxcCompiler3* DxcCompiler;
        IDxcIncludeHandler* DxcIncludeHandler;
        IDxcBlobEncoding* Source;
    };
}
