#include "wdpch.h"
#include "Pipeline.h"

Waldem::Pipeline::Pipeline(const std::string& name, const std::string& shaderName, bool bDepthTest, bool bDepthWrite, bool bBlending)
{
    Shader = new PixelShader(shaderName);
}

void Waldem::Pipeline::Bind()
{
    Shader->Bind();
}

void Waldem::Pipeline::Unbind()
{
    Shader->Unbind();
}

void Waldem::Pipeline::AddShaderParam(ShaderParamType type, const GLchar* name)
{
    Shader->AddParam(type, name);
}

void Waldem::Pipeline::SetShaderParam(const GLchar* name, void* value)
{
    Shader->SetParam(name, value);
}
