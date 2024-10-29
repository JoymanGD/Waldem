#include "wdpch.h"
#include "Shader.h"

#include <fstream>

#include "Renderer.h"
#include "..\..\Platform\Graphics\DirectX\DX12PixelShader.h"

namespace Waldem
{
    void PixelShader::SetParam(ShaderParamType type, const char* name, void* value)
    {
        if(!ShaderParameters.contains(name))
        {
            ShaderParameters[name] = new ShaderParam(type);
        }
	    
        ShaderParameters[name]->Value = value;
    }

    void PixelShader::SetBufferParam(const char* name, void* value, uint32_t size, uint32_t binding)
    {
        if(!ShaderParameters.contains(name))
        {
            ShaderParameters[name] = new ShaderParam(ShaderParamType::BUFFER, size, binding);
        }
	    
        ShaderParameters[name]->Value = value;
    }
}
