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

        for (uint32_t i = 0; i < meshes.size(); ++i)
        {
            auto mesh = meshes[i];
            
            Draw(mesh, pixelShader);
        }
    }

    PixelShader* Renderer::LoadShader(String shaderName, std::vector<Resource> resources, RenderTarget* renderTarget)
    {
        return PlatformRenderer->LoadShader(shaderName, resources, renderTarget);
    }

    Texture2D* Renderer::CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data)
    {
        Texture2D* texture = PlatformRenderer->CreateTexture(name, width, height, format, data);
        return texture;
    }

    RenderTarget* Renderer::CreateRenderTarget(String name, int width, int height, TextureFormat format)
    {
        RenderTarget* renderTarget = PlatformRenderer->CreateRenderTarget(name, width, height, format);
        return renderTarget;
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
