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
        RTYPE_RWRenderTarget = 9
    };

    struct SamplerData
    {
        void* Data;
        uint32_t Size;
    };

    class WALDEM_API Shader
    {
    public:
        Shader(const String& name) : Name(name) {}
        virtual ~Shader() = default;
        virtual void UpdateResourceData(String name, void* data) = 0;
        virtual bool CompileFromFile(const String& filepath) = 0;
    protected:
        String Name;
    };

    class WALDEM_API PixelShader : public Shader
    {
    public:
        PixelShader(const String& name, RenderTarget* renderTarget = nullptr) : Shader(name), RenderTarget(renderTarget) {}
        virtual ~PixelShader() {}
        RenderTarget* RenderTarget;
    };

    class WALDEM_API ComputeShader : public Shader
    {
    public:
        ComputeShader(const String& name) : Shader(name) {}
        virtual ~ComputeShader() {}
        virtual void* GetPlatformData() = 0;
    };
}