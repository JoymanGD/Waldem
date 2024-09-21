#include "wdpch.h"
#include "Shader.h"
#include "Renderer.h"
#include "Platform/Graphics/OpenGL/OpenGLShader.h"

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

    PixelShader* PixelShader::Create(const std::string& shaderName)
    {
        switch (Renderer::RAPI)
        {
        case RendererAPI::None:
            WD_CORE_ASSERT(false, "RendererAPI 'None' is not supported!")
            return nullptr;
        case RendererAPI::OpenGL:
            return new OpenGLPixelShader(shaderName);
        default:
            WD_CORE_ASSERT(false, "The API is not supported as an Rendering API")
            return nullptr;
        }
    }
}
