#pragma once
#include <string>
#include "Waldem/Types/WArray.h"
#include "GraphicTypes.h"
#include "RenderTarget.h"

namespace Waldem
{    
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
        RTYPE_AccelerationStructure = 10,
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
        virtual bool CompileFromFile(const String& filepath, const String& entryPoint) { throw std::runtime_error("Compilation with a single entry point is not implemented for this type of shader"); }
        virtual bool CompileFromFile(const String& filepath, const WArray<String> entryPoints) { throw std::runtime_error("Compilation with multiple entry points is not implemented for this type of shader"); }
        String GetName() { return Name; }
    protected:
        String Name;
    };

    class WALDEM_API PixelShader : public Shader
    {
    public:
        PixelShader(const String& name, const String& entryPoint) : Shader(name) {}
        virtual ~PixelShader() {}
        virtual void* GetVS() = 0;
        virtual void* GetPS() = 0;
    };

    class WALDEM_API ComputeShader : public Shader
    {
    public:
        ComputeShader(const String& name, const String& entryPoint) : Shader(name) {}
        virtual ~ComputeShader() {}
        virtual void* GetPlatformData() = 0;
    };

    class WALDEM_API RayTracingShader : public Shader
    {
    public:
        virtual bool CompileFromFile(const String& filepath) = 0;
        RayTracingShader(const String& name) : Shader(name) {}
        virtual ~RayTracingShader() {}
        virtual void* GetPlatformData() = 0;
    };
}