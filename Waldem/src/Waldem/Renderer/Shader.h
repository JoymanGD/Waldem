#pragma once
#include <map>
#include <string>

namespace Waldem
{
    enum class ShaderType
    {
        PIXEL = 0,
        COMPUTE = 1,
        RTX = 2
    };
    
    enum ResourceType
    {
        ConstantBuffer = 0,
        Buffer = 1,
        BufferRaw = 2,
        RWBuffer = 3,
        RWBufferRaw = 4,
        Texture = 5,
        RWTexture = 6,
        Sampler = 7
    };

    struct SamplerData
    {
        void* Data;
        uint32_t Size;
    };

    class PixelShader
    {
    public:
        virtual ~PixelShader() {}
        virtual void SetSamplers(std::vector<SamplerData> samplers) = 0;
        virtual void UpdateResourceData(std::string name, void* data) = 0;
    };
}