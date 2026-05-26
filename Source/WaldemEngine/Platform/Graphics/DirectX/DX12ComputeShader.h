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
        bool Reload() override;

    private:
        bool CompileFromFile(const Path& filepath, const WString& entryPoint) override;

    public:
        void* GetPlatformData() override { return ShaderBlob; }
        void Destroy() override;

    private:
        WString EntryPoint;
        IDxcBlob* ShaderBlob = nullptr;
        IDxcBlobUtf8* ErrorBlob = nullptr;
        IDxcUtils* DxcUtils = nullptr;
        IDxcCompiler3* DxcCompiler = nullptr;
        IDxcIncludeHandler* DxcIncludeHandler = nullptr;
        IDxcBlobEncoding* Source = nullptr;
    };
}
