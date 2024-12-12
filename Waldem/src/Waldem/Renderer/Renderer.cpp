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

        Instance = this;
    }

    void Renderer::Begin()
    {
        Instance->PlatformRenderer->Begin();
    }

    void Renderer::End()
    {
        Instance->PlatformRenderer->End();
    }

    void Renderer::Present()
    {
        PlatformRenderer->Present();
    }

    Point3 Renderer::GetNumThreadsPerGroup(ComputeShader* computeShader)
    {
        return Instance->PlatformRenderer->GetNumThreadsPerGroup(computeShader);
    }

    void Renderer::BeginDraw(PixelShader* pixelShader)
    {
        Instance->PlatformRenderer->BeginDraw(pixelShader);
    }

    void Renderer::Draw(Mesh* mesh)
    {
        Instance->PlatformRenderer->Draw(mesh);
    }

    void Renderer::Draw(Model* model)
    {
        Instance->PlatformRenderer->Draw(model);
    }

    void Renderer::EndDraw(PixelShader* pixelShader)
    {
        Instance->PlatformRenderer->EndDraw(pixelShader);
    }

    void Renderer::Wait()
    {
        Instance->PlatformRenderer->Wait();
    }

    void Renderer::Compute(ComputeShader* computeShader, Point3 groupCount)
    {
        Instance->PlatformRenderer->Compute(computeShader, groupCount);
    }

    PixelShader* Renderer::LoadPixelShader(String shaderName, WArray<Resource> resources, RenderTarget* renderTarget)
    {
        return Instance->PlatformRenderer->LoadPixelShader(shaderName, resources, renderTarget);
    }

    ComputeShader* Renderer::LoadComputeShader(String shaderName, WArray<Resource> resources)
    {
        return Instance->PlatformRenderer->LoadComputeShader(shaderName, resources);
    }

    Texture2D* Renderer::CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data)
    {
        Texture2D* texture = Instance->PlatformRenderer->CreateTexture(name, width, height, format, data);
        return texture;
    }

    RenderTarget* Renderer::CreateRenderTarget(String name, int width, int height, TextureFormat format)
    {
        RenderTarget* renderTarget = Instance->PlatformRenderer->CreateRenderTarget(name, width, height, format);
        return renderTarget;
    }

    VertexBuffer* Renderer::CreateVertexBuffer(void* data, uint32_t size)
    {
        return Instance->PlatformRenderer->CreateVertexBuffer(data, size);
    }

    IndexBuffer* Renderer::CreateIndexBuffer(void* data, uint32_t count)
    {
        return Instance->PlatformRenderer->CreateIndexBuffer(data, count);
    }
}
