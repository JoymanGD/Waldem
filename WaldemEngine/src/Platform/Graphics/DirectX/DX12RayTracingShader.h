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

    private:
        bool CompileFromFile(const Path& filepath) override;

    public:
        void* GetPlatformData() override { return ShaderBlob; }
        void Destroy() override { if (ShaderBlob) ShaderBlob->Release(); if (ErrorBlob) ErrorBlob->Release(); if (DxcUtils) DxcUtils->Release(); if (DxcCompiler) DxcCompiler->Release(); if (DxcIncludeHandler) DxcIncludeHandler->Release(); }

    private:
        IDxcBlob* ShaderBlob;
        IDxcBlobUtf8* ErrorBlob;
        IDxcUtils* DxcUtils;
        IDxcCompiler3* DxcCompiler;
        IDxcIncludeHandler* DxcIncludeHandler;
        IDxcBlobEncoding* Source;
    };
}
