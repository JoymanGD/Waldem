#pragma once
#include <string>
#include "RenderTarget.h"

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
        RTYPE_ConstantBuffer = 0,
        RTYPE_Buffer = 1,
        RTYPE_BufferRaw = 2,
        RTYPE_RWBuffer = 3,
        RTYPE_RWBufferRaw = 4,
        RTYPE_Texture = 5,
        RTYPE_RWTexture = 6,
        RTYPE_Sampler = 7,
        RTYPE_RenderTarget = 8,
    };

    struct SamplerData
    {
        void* Data;
        uint32_t Size;
    };

    class WALDEM_API PixelShader
    {
    public:
        PixelShader(const String& name, RenderTarget* renderTarget = nullptr) : Name(name), RenderTarget(renderTarget) {}
        virtual ~PixelShader() {}
        virtual void SetSamplers(std::vector<SamplerData> samplers) = 0;
        virtual void UpdateResourceData(String name, void* data) = 0;
        
        RenderTarget* RenderTarget;
    protected:
        String Name;
    };
}