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
        Instance->PlatformRenderer->Present();
    }

    Point3 Renderer::GetNumThreadsPerGroup(ComputeShader* computeShader)
    {
        return Instance->PlatformRenderer->GetNumThreadsPerGroup(computeShader);
    }

    void Renderer::Draw(Mesh* mesh)
    {
        Instance->PlatformRenderer->Draw(mesh);
    }

    void Renderer::Draw(Model* model)
    {
        Instance->PlatformRenderer->Draw(model);
    }

    void Renderer::DrawLine(Line line)
    {
        Instance->PlatformRenderer->DrawLine(line);
    }

    void Renderer::DrawLines(WArray<Line> lines)
    {
        Instance->PlatformRenderer->DrawLines(lines);
    }

    void Renderer::Wait()
    {
        Instance->PlatformRenderer->Wait();
    }

    void Renderer::Compute(Point3 groupCount)
    {
        Instance->PlatformRenderer->Compute(groupCount);
    }

    PixelShader* Renderer::LoadPixelShader(String shaderName, String entryPoint)
    {
        return Instance->PlatformRenderer->LoadPixelShader(shaderName, entryPoint);
    }

    ComputeShader* Renderer::LoadComputeShader(String shaderName, String entryPoint)
    {
        return Instance->PlatformRenderer->LoadComputeShader(shaderName, entryPoint);
    }

    void Renderer::SetPipeline(Pipeline* pipeline)
    {
        Instance->PlatformRenderer->SetPipeline(pipeline);
    }

    void Renderer::SetRootSignature(RootSignature* rootSignature)
    {
        Instance->PlatformRenderer->SetRootSignature(rootSignature);
    }

    void Renderer::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil)
    {
        Instance->PlatformRenderer->SetRenderTargets(renderTargets, depthStencil);
    }

    Pipeline* Renderer::CreateGraphicPipeline(const String& name, WArray<TextureFormat> RTFormats, PrimitiveTopologyType primitiveTopologyType, RootSignature* rootSignature, PixelShader* shader)
    {
        return Instance->PlatformRenderer->CreateGraphicPipeline(name, RTFormats, primitiveTopologyType, rootSignature, shader);
    }

    Pipeline* Renderer::CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader)
    {
        return Instance->PlatformRenderer->CreateComputePipeline(name, rootSignature, shader);
    }

    RootSignature* Renderer::CreateRootSignature(WArray<Resource> resources)
    {
        return Instance->PlatformRenderer->CreateRootSignature(resources);
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

    void Renderer::CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT)
    {
        Instance->PlatformRenderer->CopyRenderTarget(dstRT, srcRT);
    }

    Buffer* Renderer::CreateBuffer(BufferType type, void* data, uint32_t size)
    {
        return Instance->PlatformRenderer->CreateBuffer(type, data, size);
    }

    void Renderer::ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after)
    {
        Instance->PlatformRenderer->ResourceBarrier(rt, before, after);
    }

    void Renderer::ResourceBarrier(Buffer* buffer, ResourceStates before, ResourceStates after)
    {
        Instance->PlatformRenderer->ResourceBarrier(buffer, before, after);
    }

    void Renderer::ClearRenderTarget(RenderTarget* rt)
    {
        Instance->PlatformRenderer->ClearRenderTarget(rt);
    }

    void Renderer::ClearDepthStencil(RenderTarget* ds)
    {
        Instance->PlatformRenderer->ClearDepthStencil(ds);
    }

    void Renderer::InitializeUI()
    {
        Instance->PlatformRenderer->InitializeUI();
    }

    void Renderer::BeginUI()
    {
        Instance->PlatformRenderer->BeginUI();
    }

    void Renderer::EndUI()
    {
        Instance->PlatformRenderer->EndUI();
    }
}