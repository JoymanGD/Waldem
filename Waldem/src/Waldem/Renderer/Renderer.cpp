#include "wdpch.h"
#include "Renderer.h"
#include "Platform/Graphics/OpenGL/OpenGLRenderer.h"

namespace Waldem
{
    RendererAPI Renderer::RAPI = RendererAPI::OpenGL;

    void Renderer::Initialize()
    {
        switch (RAPI)
        {
        case RendererAPI::None:
            break;
        case RendererAPI::OpenGL:
            CurrentRenderer = (IRenderer*)new OpenGLRenderer();
            break;
        case RendererAPI::DirectX:
            break;
        case RendererAPI::Vulkan:
            break;
        }

        CurrentRenderer->Initialize();
    }

    void Renderer::Clear()
    {
        CurrentRenderer->Clear(ClearColor);
    }

    void Renderer::DrawMesh(Mesh* mesh)
    {
        mesh->VA->Bind();
        mesh->MeshMaterial.Bind();
        CurrentRenderer->Render(mesh->VA->GetIndexBuffer()->GetCount());
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
