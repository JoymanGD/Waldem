#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12ComputeShader.h"
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
#define MAX_TEXTURES 1024
#define MAX_BUFFERS 128
    
    DX12ComputeShader::DX12ComputeShader(const String& name) : ComputeShader(name)
    {
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcUtils));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create DxcUtils.");
        }

        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcCompiler));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create DxcCompiler.");
        }

        hr = DxcUtils->CreateDefaultIncludeHandler(&DxcIncludeHandler);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create include handler.");
        }
        
        CompileFromFile(name);
    }

    DX12ComputeShader::~DX12ComputeShader()
    {
    }

    bool DX12ComputeShader::CompileFromFile(const String& shaderName)
    {
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderName = std::wstring(shaderName.begin(), shaderName.end());
        
        size_t lastSlash = wShaderName.find_last_of(L"/\\");

        std::wstring pathToShaders = wCurrentPath + L"/Shaders/";

        std::wstring baseName = wShaderName.substr(lastSlash + 1);
        
        std::wstring shaderPath = pathToShaders + baseName + L".comp.hlsl";

        HRESULT hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        const wchar_t* entryPoint = L"main";
        const wchar_t* targetProfile = L"cs_6_5";

        // Compiler arguments
        const wchar_t* arguments[] = {
            L"-E", entryPoint,
            L"-T", targetProfile,
            L"-Zi",
            L"-Qstrip_debug",
            L"-I", pathToShaders.c_str()
        };

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = Source->GetBufferPointer();
        sourceBuffer.Size = Source->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        IDxcResult* result;

        DxcCompiler->Compile(
            &sourceBuffer,
            arguments,
            _countof(arguments),
            DxcIncludeHandler,
            IID_PPV_ARGS(&result)
        );

        HRESULT status;
        
        hr = result->GetStatus(&status);
        
        if (FAILED(hr) || FAILED(status))
        {
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&ErrorBlob), nullptr);
            if (ErrorBlob && ErrorBlob->GetStringLength() > 0)
            {
                std::cerr << "Shader compilation error: " << ErrorBlob->GetStringPointer() << std::endl;
            }
            throw std::runtime_error("Shader compilation failed.");
        }

        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderBlob), nullptr);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to retrieve compiled shader.");
        }

        return true;
    }
}
