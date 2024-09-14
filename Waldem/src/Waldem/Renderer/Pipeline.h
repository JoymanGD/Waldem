#pragma once
#include "PixelShader.h"

namespace Waldem
{
    class Pipeline
    {
    public:
        Pipeline(const std::string& name, const std::string& shaderName, bool bDepthTest, bool bDepthWrite, bool bBlending);
        ~Pipeline() = default;
        
        PixelShader* Shader;

        void Bind();
        void Unbind();
        
        void AddShaderParam(ShaderParamType type, const GLchar* name);
        void SetShaderParam(const GLchar* name, void* value);
    };
}
