#pragma once
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class OpenGLPixelShader : public PixelShader
    {
    public:
        OpenGLPixelShader(const std::string& shaderName);
        ~OpenGLPixelShader() override;
        void Bind() const override;
        void Unbind() const override;

    private:
        std::string LoadShaderFile(std::string& filename);
        void InitializeShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        
        uint32_t VS;
        uint32_t PS;
        uint32_t ProgramID;
    };
}