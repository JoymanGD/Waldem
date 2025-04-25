#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12PixelShader.h"

#include <regex>
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    DX12PixelShader::DX12PixelShader(const Path& name, const WString& entryPoint) : PixelShader(name, entryPoint)
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
        
        CompileFromFile(name, entryPoint);
    }

    DX12PixelShader::~DX12PixelShader()
    {
    }

    bool DX12PixelShader::CompileFromFile(const Path& path, const WString& entryPoint)
    {
        auto currentPath = GetCurrentFolder();
        
        // Extract the directory part
        std::wstring wCurrentPath = currentPath;
        std::wstring wShaderFolder = path.parent_path();
        std::wstring wShaderName = path.filename();

        std::wstring pathToShaders = wCurrentPath + L"\\" + L"Shaders";
        pathToShaders = std::regex_replace(pathToShaders, std::wregex(L"[\\/\\\\]"), L"\\\\");
        std::wstring currentShaderFolderPath = pathToShaders + L"\\\\" + wShaderFolder;
        std::wstring shaderPath = currentShaderFolderPath + (wShaderFolder.empty() ? L"" : L"\\\\") + wShaderName + L".vs.hlsl";
        
        HRESULT hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        const wchar_t* entryPointW = DX12Helper::WFromMB(entryPoint);
        const wchar_t* targetProfile = L"vs_6_5";

        // Compiler arguments
        const wchar_t* arguments[] = {
            L"-E", entryPointW,
            L"-T", targetProfile,
            L"-Zi",
            L"-Qembed_debug",
            L"-I", pathToShaders.c_str(),
            L"-D", L"_DXC_COMPILER"
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

        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&VertexShaderBlob), nullptr);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to retrieve compiled shader.");
        }

        //pixel shader
        shaderPath = currentShaderFolderPath + (wShaderFolder.empty() ? L"" : L"\\\\") + wShaderName + L".ps.hlsl";
        
        hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        entryPointW = DX12Helper::WFromMB(entryPoint);
        targetProfile = L"ps_6_5";

        // Compiler arguments
        const wchar_t* psArguments[] = {
            L"-E", entryPointW,
            L"-T", targetProfile,
            L"-Zi",
            L"-Qembed_debug",
            L"-I", pathToShaders.c_str(),
            L"-D", L"_DXC_COMPILER"
        };

        sourceBuffer.Ptr = Source->GetBufferPointer();
        sourceBuffer.Size = Source->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        DxcCompiler->Compile(
            &sourceBuffer,
            psArguments,
            _countof(psArguments),
            DxcIncludeHandler,
            IID_PPV_ARGS(&result)
        );

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

        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&PixelShaderBlob), nullptr);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to retrieve compiled shader.");
        }

        return true;
    }
}
