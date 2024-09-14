#include "wdpch.h"
#include "Renderer.h"

#include <backends/imgui_impl_opengl3_loader.h>

namespace Waldem
{
    RendererAPI Renderer::RAPI = RendererAPI::OpenGL;

    void Renderer::Initialize()
    {
		glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void Renderer::DrawMesh(Mesh* mesh)
    {
        mesh->VA->Bind();
        mesh->MeshMaterial.Bind();
		glDrawElements(GL_TRIANGLES, mesh->VA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        mesh->MeshMaterial.Unbind();
        mesh->VA->Unbind();
    }

    void Renderer::DrawMesh(Pipeline* pipeline, Mesh* mesh)
    {
        pipeline->Bind();

        DrawMesh(mesh);
        
        pipeline->Unbind();
    }

    void Renderer::DrawModel(Pipeline* pipeline, Model* model)
    {
        pipeline->Bind();

        auto meshes = model->GetMeshes();
        
        for (auto mesh : meshes)
        {
            DrawMesh(mesh);
        }
        
        pipeline->Unbind();
    }
}
