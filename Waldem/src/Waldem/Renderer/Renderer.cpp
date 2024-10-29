#include "wdpch.h"
#include "Renderer.h"

#include "Platform/Graphics/DirectX/DirectXRenderer.h"
#include "Platform/Graphics/DirectX/DX12PixelShader.h"

namespace Waldem
{
    RendererAPI Renderer::RAPI = RendererAPI::DirectX;

    void Renderer::Initialize(Window* window)
    {
        switch (RAPI)
        {
        case RendererAPI::None:
            break;
        case RendererAPI::DirectX: 
            CurrentRenderer = (IRenderer*)new DirectXRenderer();
            break;
        case RendererAPI::Vulkan:
            break;
        }

        CurrentRenderer->Initialize(window);
    }

    void Renderer::Begin(uint32_t frame)
    {
        CurrentRenderer->SetFrameIndex(frame);

        CurrentRenderer->Begin();
    }

    void Renderer::End()
    {
        CurrentRenderer->End();
    }

    void Renderer::DrawMesh(Mesh* mesh, PixelShader* pixelShader)
    {
        CurrentRenderer->DrawMesh(mesh, pixelShader);
    }

    void Renderer::DrawModel(Model* model, PixelShader* pixelShader)
    {
        auto meshes = model->GetMeshes();
        
        for (auto mesh : meshes)
        {
            DrawMesh(mesh, pixelShader);
        }
    }

    PixelShader* Renderer::LoadShader(std::string shaderName)
    {
        return CurrentRenderer->LoadShader(shaderName);
    }
}
