#pragma once
#include "glad/glad.h"

namespace Waldem
{
    enum class ShaderParamType
    {
        FLOAT = 0,
        MAT4 = 1,
        VEC3 = 2,
        UINT = 3,
    };

    struct ShaderParam
    {
        ShaderParamType Type;
        const GLchar* Name;
        void* Value;

        ShaderParam(ShaderParamType type, const GLchar* name, void* value)
        {
            Type = type;
            Name = name;
            Value = value;
        }
    };
    
    enum class ShaderType
    {
        PIXEL = 0,
        COMPUTE = 1,
        RTX = 2
    };
    
    class PixelShader
    {
    public:
        PixelShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        PixelShader(const std::string& shaderName);
        ~PixelShader();

        void Bind(std::vector<ShaderParam*>& shaderParams);
        void Unbind(std::vector<ShaderParam*>& shaderParams);

    private:
        std::string LoadShaderFile(std::string& filename);
        void InitializeShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        
        GLuint VS;
        GLuint PS;
        uint32_t RendererID;
    };
}
