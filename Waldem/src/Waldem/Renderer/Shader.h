#pragma once
#include <string>

#include "GraphicTypes.h"
#include "RenderTarget.h"

namespace Waldem
{
    enum class PipelineType
    {
        Graphics = 0,
        Compute = 1
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
        RTYPE_RWRenderTarget = 9,
        RTYPE_Constant = 19
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
        virtual bool CompileFromFile(const String& filepath) = 0;
        String GetName() { return Name; }
    protected:
        String Name;
    };

    class WALDEM_API PixelShader : public Shader
    {
    public:
        PixelShader(const String& name) : Shader(name) {}
        virtual ~PixelShader() {}
        virtual void* GetVS() = 0;
        virtual void* GetPS() = 0;
    };

    class WALDEM_API ComputeShader : public Shader
    {
    public:
        ComputeShader(const String& name) : Shader(name) {}
        virtual ~ComputeShader() {}
        virtual void* GetPlatformData() = 0;
    };
}