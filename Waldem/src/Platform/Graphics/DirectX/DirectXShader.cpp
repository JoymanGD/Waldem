#include "wdpch.h"
#include <d3dcompiler.h>
#include "DirectXShader.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    DirectXPixelShader::DirectXPixelShader(const std::string& shaderName)
    {
        if(CompileFromFile(shaderName))
        {
            
        }
    }

    DirectXPixelShader::~DirectXPixelShader()
    {
    }

    void DirectXPixelShader::Bind() const
    {
    }

    void DirectXPixelShader::Unbind() const
    {
    }

    bool DirectXPixelShader::CompileFromFile(const std::string& shaderName)
    {
        std::string entryPoint = "main"; //TODO: make this configurable?
        
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderName = std::wstring(shaderName.begin(), shaderName.end());
        
        size_t lastSlash = wShaderName.find_last_of(L"/\\");

        // Extract the directory part
        std::wstring pathToShaders = wCurrentPath + L"/Shaders/";

        // Extract the base name
        std::wstring baseName = wShaderName.substr(lastSlash + 1);
        
        std::wstring shaderPath = pathToShaders + baseName + L".vert.hlsl";
        std::string target = "vs_5_0";

        //vertex shader
        HRESULT hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &vertexShader, // Output shader bytecode
            &errorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (errorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)errorBlob->GetBufferPointer());
            }
            
            return false;
        }

        //pixel shader
        shaderPath = pathToShaders + baseName + L".frag.hlsl";
        target = "ps_5_0";
        
        hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &pixelShader, // Output shader bytecode
            &errorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (errorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)errorBlob->GetBufferPointer());
            }
            
            return false;
        }

        return true;
    }

    bool DirectXPixelShader::CreatePipelineState(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
    {
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        
        if (FAILED(hr))
        {
            WD_CORE_ERROR("Failed to create PSO!");
            return false;
        }
    
        return true;
    }
}