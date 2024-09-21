#pragma once
#include "Shader.h"

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
        
        void SetShaderParam(ShaderParamType type, const char* name, void* value);
        void SetShaderBufferParam(const char* name, void* value, uint32_t size, uint32_t binding);
    };
}
