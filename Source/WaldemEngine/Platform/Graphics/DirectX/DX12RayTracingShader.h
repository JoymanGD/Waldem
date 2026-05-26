#pragma once
#include <d3dcommon.h>
#include <dxcapi.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12RayTracingShader : public RayTracingShader
    {
    public:
        DX12RayTracingShader(const Path& name); 
        ~DX12RayTracingShader() override;
        bool Reload() override;

    private:
        bool CompileFromFile(const Path& filepath) override;

    public:
        void* GetPlatformData() override { return ShaderBlob; }
        void Destroy() override;

    private:
        IDxcBlob* ShaderBlob = nullptr;
        IDxcBlobUtf8* ErrorBlob = nullptr;
        IDxcUtils* DxcUtils = nullptr;
        IDxcCompiler3* DxcCompiler = nullptr;
        IDxcIncludeHandler* DxcIncludeHandler = nullptr;
        IDxcBlobEncoding* Source = nullptr;
    };
}
