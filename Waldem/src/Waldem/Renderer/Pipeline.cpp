#include "wdpch.h"
#include "Pipeline.h"

namespace Waldem
{
    Pipeline::Pipeline(const std::string& name, const std::string& shaderName, bool bDepthTest, bool bDepthWrite, bool bBlending)
    {
        Shader = PixelShader::Create(shaderName);
    }

    void Pipeline::Bind()
    {
        Shader->Bind();
    }

    void Pipeline::Unbind()
    {
        Shader->Unbind();
    }

    void Pipeline::SetShaderParam(ShaderParamType type, const char* name, void* value)
    {
        Shader->SetParam(type, name, value);
    }
    
    void Pipeline::SetShaderBufferParam(const char* name, void* value, uint32_t size, uint32_t binding)
    {
        Shader->SetBufferParam(name, value, size, binding);
    }
}
