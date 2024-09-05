#include "wdpch.h"
#include "Renderer.h"

#include <backends/imgui_impl_opengl3_loader.h>

namespace Waldem
{
    RendererAPI Renderer::RAPI = RendererAPI::OpenGL;

    void Renderer::DrawMesh(Mesh* mesh)
    {
        mesh->Bind();
		glDrawElements(GL_TRIANGLES, mesh->VA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        mesh->Unbind();
    }
}
