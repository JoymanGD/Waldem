#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12Shader.h"
#include "DX12ComputeShader.h"
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"
#include <regex>

namespace Waldem
{
    namespace
    {
        template<typename T>
        void ReleaseCom(T*& object)
        {
            if(object != nullptr)
            {
                object->Release();
                object = nullptr;
            }
        }
    }

    DX12ComputeShader::DX12ComputeShader(const Path& name, const WString& entryPoint) : ComputeShader(name, entryPoint), EntryPoint(entryPoint)
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

    DX12ComputeShader::~DX12ComputeShader()
    {
    }

    void DX12ComputeShader::Destroy()
    {
        ReleaseCom(ShaderBlob);
        ReleaseCom(ErrorBlob);
        ReleaseCom(Source);
        ReleaseCom(DxcIncludeHandler);
        ReleaseCom(DxcCompiler);
        ReleaseCom(DxcUtils);
    }

    bool DX12ComputeShader::Reload()
    {
        try
        {
            return CompileFromFile(Name, EntryPoint);
        }
        catch(const std::exception& exception)
        {
            WD_CORE_ERROR("Failed to hot reload compute shader '{0}': {1}", Name.string(), exception.what());
            return false;
        }
    }

    bool DX12ComputeShader::CompileFromFile(const Path& path, const WString& entryPoint)
    {
        const Path computeShaderPath = ResolveShaderFilePath(path, L".comp.hlsl");
        const std::wstring pathToShaders = computeShaderPath.parent_path().wstring();
        const std::wstring currentShaderFolderPath = computeShaderPath.parent_path().wstring();
        std::wstring shaderPath = computeShaderPath.wstring();

        HRESULT hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        const wchar_t* entryPointW = DX12Helper::WFromMB(entryPoint);
        const wchar_t* targetProfile = L"cs_6_6";

        // Compiler arguments
        const wchar_t* arguments[] = {
            L"-E", entryPointW,
            L"-T", targetProfile,
            L"-Zi",
            L"-Qembed_debug",
            L"-I", pathToShaders.c_str(),
            L"-I", currentShaderFolderPath.c_str(),
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

        IDxcBlob* compiledShaderBlob = nullptr;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to retrieve compiled shader.");
        }

        ReleaseCom(ShaderBlob);
        ShaderBlob = compiledShaderBlob;

        return true;
    }
}
