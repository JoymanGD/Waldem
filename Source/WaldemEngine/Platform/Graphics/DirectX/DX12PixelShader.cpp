#include "wdpch.h"
#include "DX12PixelShader.h"

#include <regex>
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"

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

    DX12PixelShader::DX12PixelShader(const Path& name, const WString& entryPoint) : PixelShader(name, entryPoint), EntryPoint(entryPoint)
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

    void DX12PixelShader::Destroy()
    {
        ReleaseCom(VertexShaderBlob);
        ReleaseCom(PixelShaderBlob);
        ReleaseCom(ErrorBlob);
        ReleaseCom(Source);
        ReleaseCom(DxcIncludeHandler);
        ReleaseCom(DxcCompiler);
        ReleaseCom(DxcUtils);
    }

    bool DX12PixelShader::Reload()
    {
        try
        {
            return CompileFromFile(Name, EntryPoint);
        }
        catch(const std::exception& exception)
        {
            WD_CORE_ERROR("Failed to hot reload pixel shader '{0}': {1}", Name.string(), exception.what());
            return false;
        }
    }

    bool DX12PixelShader::CompileFromFile(const Path& path, const WString& entryPoint)
    {
        const Path vertexShaderPath = ResolveShaderFilePath(path, L".vs.hlsl");
        const Path pixelShaderPath = ResolveShaderFilePath(path, L".ps.hlsl");
        const std::wstring pathToShaders = vertexShaderPath.parent_path().wstring();
        std::wstring shaderPath = vertexShaderPath.wstring();
        
        HRESULT hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        const wchar_t* entryPointW = DX12Helper::WFromMB(entryPoint);
        const wchar_t* targetProfile = L"vs_6_6";

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

        IDxcBlob* compiledVertexShaderBlob = nullptr;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledVertexShaderBlob), nullptr);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to retrieve compiled shader.");
        }

        //pixel shader
        shaderPath = pixelShaderPath.wstring();
        
        hr = DxcUtils->LoadFile(shaderPath.c_str(), nullptr, &Source);

        if(FAILED(hr))
        {
            WD_CORE_ERROR("Failed to load shader file: {0}", DX12Helper::MBFromW(shaderPath.c_str(), 0));
        }

        entryPointW = DX12Helper::WFromMB(entryPoint);
        targetProfile = L"ps_6_6";

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

        IDxcBlob* compiledPixelShaderBlob = nullptr;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledPixelShaderBlob), nullptr);
        if (FAILED(hr))
        {
            ReleaseCom(compiledVertexShaderBlob);
            throw std::runtime_error("Failed to retrieve compiled shader.");
        }

        ReleaseCom(VertexShaderBlob);
        ReleaseCom(PixelShaderBlob);
        VertexShaderBlob = compiledVertexShaderBlob;
        PixelShaderBlob = compiledPixelShaderBlob;

        return true;
    }
}
