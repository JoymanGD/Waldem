#include "wdpch.h"
#include "Renderer.h"

#include "Platform/Graphics/DirectX/DX12PixelShader.h"
#include "Platform/Graphics/DirectX/DX12Renderer.h"

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
            PlatformRenderer = (IRenderer*)new DX12Renderer();
            break;
        case RendererAPI::Vulkan:
            throw std::runtime_error("Vulkan is not supported yet!");
        }

        PlatformRenderer->Initialize(window);
    }

    void Renderer::Begin()
    {
        PlatformRenderer->Begin();
    }

    void Renderer::End()
    {
        PlatformRenderer->End();
    }

    void Renderer::Present()
    {
        PlatformRenderer->Present();
    }

    void Renderer::Draw(Mesh* mesh, PixelShader* pixelShader)
    {
        PlatformRenderer->Draw(mesh, pixelShader);
    }

    void Renderer::Draw(Model* model, PixelShader* pixelShader)
    {
        auto meshes = model->GetMeshes();
        
        for (auto mesh : meshes)
        {
            Draw(mesh, pixelShader);
        }
    }

    PixelShader* Renderer::LoadShader(std::string shaderName, std::vector<ResourceDesc> resources)
    {
        return PlatformRenderer->LoadShader(shaderName, resources);
    }

    Texture2D* Renderer::CreateTexture(std::string name, int width, int height, int channels, uint8_t* data)
    {
        return PlatformRenderer->CreateTexture(name, width, height, channels, data);
    }

    VertexBuffer* Renderer::CreateVertexBuffer(void* data, uint32_t size)
    {
        return PlatformRenderer->CreateVertexBuffer(data, size);
    }

    IndexBuffer* Renderer::CreateIndexBuffer(void* data, uint32_t count)
    {
        return PlatformRenderer->CreateIndexBuffer(data, count);
    }
}
