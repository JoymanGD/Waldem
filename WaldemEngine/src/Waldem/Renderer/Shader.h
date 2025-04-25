#pragma once
#include "Waldem/Types/WArray.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    struct SamplerData
    {
        void* Data;
        uint32_t Size;
    };

    class WALDEM_API Shader
    {
    public:
        Shader(const Path& name) : Name(name) {}
        virtual ~Shader() = default;
        virtual void Destroy() = 0;
        virtual bool CompileFromFile(const Path& filepath, const WString& entryPoint) { throw std::runtime_error("Compilation with a single entry point is not implemented for this type of shader"); }
        virtual bool CompileFromFile(const Path& filepath, const WArray<WString> entryPoints) { throw std::runtime_error("Compilation with multiple entry points is not implemented for this type of shader"); }
        Path GetName() { return Name; }
    protected:
        Path Name;
    };

    class WALDEM_API PixelShader : public Shader
    {
    public:
        PixelShader(const Path& name, const WString& entryPoint) : Shader(name) {}
        virtual ~PixelShader() {}
        virtual void* GetVS() = 0;
        virtual void* GetPS() = 0;
    };

    class WALDEM_API ComputeShader : public Shader
    {
    public:
        ComputeShader(const Path& name, const WString& entryPoint) : Shader(name) {}
        virtual ~ComputeShader() {}
        virtual void* GetPlatformData() = 0;
    };

    class WALDEM_API RayTracingShader : public Shader
    {
    public:
        virtual bool CompileFromFile(const Path& filepath) = 0;
        RayTracingShader(const Path& name) : Shader(name) {}
        virtual ~RayTracingShader() {}
        virtual void* GetPlatformData() = 0;
    };
}