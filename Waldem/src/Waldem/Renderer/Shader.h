#pragma once
#include <map>
#include <string>

namespace Waldem
{
    enum class ShaderParamType
    {
        FLOAT = 0,
        FLOAT2 = 1,
        FLOAT3 = 2,
        FLOAT4 = 3,
        MAT3 = 4,
        MAT4 = 5,
        UINT = 6,
        UINT2 = 7,
        UINT3 = 8,
        UINT4 = 9,
        TEXTURE2D = 10,
        BUFFER = 11,
    };

    struct ShaderParam
    {
        ShaderParamType Type;
        void* Value;
        uint32_t Size;
        uint32_t Binding;

        ShaderParam(ShaderParamType type, uint32_t size = 0, uint32_t binding = 0) : Type(type), Value(nullptr), Size(size), Binding(binding) {}
        ShaderParam(ShaderParamType type, void* value, uint32_t size = 0, uint32_t binding = -1) : Type(type), Value(value), Size(size), Binding(binding) {}
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
        virtual ~PixelShader() {}
        void SetParam(ShaderParamType type, const char* name, void* value);
        void SetBufferParam(const char* name, void* value, uint32_t size, uint32_t binding);

    protected:
        std::map<std::string, ShaderParam*> ShaderParameters;
    };
}