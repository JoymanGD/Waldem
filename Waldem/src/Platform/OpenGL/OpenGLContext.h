#pragma once
#include "Waldem/Renderer/RenderingContext.h"

struct GLFWwindow;

namespace Waldem
{
    class OpenGLContext : public RenderingContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);
        virtual void Init() override;
        virtual void SwapBuffers() override;
        virtual void LogContextInfo() override;
        std::string GetContextName() override;

    private:
        GLFWwindow* WindowHandle;
    };
}