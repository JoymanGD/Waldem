#pragma once
#include <map>

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
        void* Value;

        ShaderParam(ShaderParamType type, const GLchar* name) : Type(type), Value(nullptr) {}
        ShaderParam(ShaderParamType type, const GLchar* name, void* value) : Type(type), Value(value) {}
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

        void Bind();
        void Unbind();
        
        void AddParam(ShaderParamType type, const GLchar* name);
        void SetParam(const GLchar* name, void* value);

        uint32_t GetProgramID() { return ProgramID; }

    private:
        std::string LoadShaderFile(std::string& filename);
        void InitializeShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        // std::vector<ShaderParam*> ShaderParameters;
        std::map<std::string, ShaderParam*> ShaderParameters;
        
        GLuint VS;
        GLuint PS;
        uint32_t ProgramID;
    };
}
