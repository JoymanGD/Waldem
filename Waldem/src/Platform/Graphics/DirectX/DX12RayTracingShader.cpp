#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12RayTracingShader.h"
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"
#include <regex>

namespace Waldem
{    
    DX12RayTracingShader::DX12RayTracingShader(const String& name) : RayTracingShader(name)
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

    DX12RayTracingShader::~DX12RayTracingShader()
    {
    }

    bool DX12RayTracingShader::CompileFromFile(const String& shaderName)
    {
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderFolder = shaderName.find_last_of('/') != std::string::npos ? std::wstring(shaderName.begin(), shaderName.begin() + shaderName.find_last_of('/')) : L"";
        std::wstring wShaderName = wShaderFolder.empty() ? std::wstring(shaderName.begin(), shaderName.end()) : std::wstring(shaderName.begin() + shaderName.find_last_of('/') + 1, shaderName.end());

        std::wstring pathToShaders = wCurrentPath + L"\\" + L"Shaders";
        pathToShaders = std::regex_replace(pathToShaders, std::wregex(L"[\\/\\\\]"), L"\\\\");
        std::wstring currentShaderFolderPath = pathToShaders + L"\\\\" + wShaderFolder;
        std::wstring shaderPath = currentShaderFolderPath + (wShaderFolder.empty() ? L"" : L"\\\\") + wShaderName + L".rtx.hlsl";

        HRESULT hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        const wchar_t* targetProfile = L"lib_6_5";

        // Compiler arguments
        const wchar_t* arguments[] = {
            L"-T", targetProfile,
            L"-Zi",
            L"-Qembed_debug",
            L"-I", pathToShaders.c_str(),
            L"-I", currentShaderFolderPath.c_str(),
            L"-D", L"_DXC_COMPILER",
            L"-D", L"ENABLE_RAY_TRACING"
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
