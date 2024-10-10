#include "wdpch.h"
#include "OpenGLContext.h"
#include <GLFW/glfw3.h>

#include "glad/glad.h"

namespace Waldem
{
    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle) : WindowHandle(windowHandle)
    {
        WD_CORE_ASSERT(WindowHandle, "Window handle is invalid!")
    }

    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(WindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        WD_CORE_ASSERT(status, "Failed to initialize Glad!")
    }

    void OpenGLContext::SwapBuffers()
    {
		glfwSwapBuffers(WindowHandle);
    }

    void OpenGLContext::LogContextInfo()
    {
        WD_CORE_INFO("Device: {0}", glGetString(GL_RENDERER));
        WD_CORE_INFO("Version: {0}", glGetString(GL_VERSION));
    }

    std::string OpenGLContext::GetContextName()
    {
        return "OpenGL";
    }
}