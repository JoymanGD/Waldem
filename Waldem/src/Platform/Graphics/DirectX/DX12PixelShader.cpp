#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12PixelShader.h"
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
#define MAX_TEXTURES 1024
#define MAX_BUFFERS 128
    
    DX12PixelShader::DX12PixelShader(const String& name) : PixelShader(name)
    {
        CompileFromFile(name);
    }

    DX12PixelShader::~DX12PixelShader()
    {
    }

    bool DX12PixelShader::CompileFromFile(const String& shaderName)
    {
        String entryPoint = "main"; //TODO: make this configurable?
        
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderName = std::wstring(shaderName.begin(), shaderName.end());
        
        size_t lastSlash = wShaderName.find_last_of(L"/\\");

        // Extract the directory part
        std::wstring pathToShaders = wCurrentPath + L"/Shaders/";

        // Extract the base name
        std::wstring baseName = wShaderName.substr(lastSlash + 1);
        
        std::wstring shaderPath = pathToShaders + baseName + L".vs.hlsl";
        String target = "vs_5_1";

        //vertex shader
        HRESULT hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &VertexShaderBlob, // Output shader bytecode
            &ErrorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (ErrorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)ErrorBlob->GetBufferPointer());
            }
            
            return false;
        }

        //pixel shader
        shaderPath = pathToShaders + baseName + L".ps.hlsl";
        target = "ps_5_1";
        
        hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &PixelShaderBlob, // Output shader bytecode
            &ErrorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (ErrorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)ErrorBlob->GetBufferPointer());
            }
            
            return false;
        }

        return true;
    }
}
