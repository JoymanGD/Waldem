#pragma once
#include <d3dcommon.h>
#include <dxcapi.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12RayTracingShader : public RayTracingShader
    {
    public:
        DX12RayTracingShader(const String& name); 
        ~DX12RayTracingShader() override;

    private:
        bool CompileFromFile(const String& filepath) override;

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
