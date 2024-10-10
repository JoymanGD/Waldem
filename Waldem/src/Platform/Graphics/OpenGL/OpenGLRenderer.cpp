#include "wdpch.h"
#include "OpenGLRenderer.h"
#include "glad/glad.h"

namespace Waldem
{
    void OpenGLRenderer::Initialize(Window* window)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void OpenGLRenderer::Clear(Vector4 clearColor)
    {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLRenderer::Render(uint32_t indexCount)
    {
	    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }
}