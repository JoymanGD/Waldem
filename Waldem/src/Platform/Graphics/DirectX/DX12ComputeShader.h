#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "DX12CommandList.h"
#include "DX12Resource.h"
#include "DX12Shader.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12ComputeShader : public ComputeShader
    {
    public:
        DX12ComputeShader(const String& name);
        ~DX12ComputeShader() override;

    private:
        bool CompileFromFile(const String& filepath) override;

    public:
        void* GetPlatformData() override { return ShaderBlob; }

    private:
        ID3DBlob* ShaderBlob;
        ID3DBlob* ErrorBlob;
    };
}
