#pragma once

namespace Waldem
{
    class RenderingContext
    {
    public:
        virtual ~RenderingContext() = default;
        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;
        virtual void LogContextInfo() = 0;
        virtual std::string GetContextName() = 0;
    };
}