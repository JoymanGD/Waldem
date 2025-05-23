#include "wdpch.h"
#include "Renderer.h"
#include "Platform/Graphics/DirectX/DX12PixelShader.h"
#include "Platform/Graphics/DirectX/DX12Renderer.h"

namespace Waldem
{
    RendererAPI Renderer::RAPI = RendererAPI::DirectX;

    void Renderer::Initialize(CWindow* window)
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

    void Renderer::Draw(CMesh* mesh)
    {
        Instance->PlatformRenderer->Draw(mesh);
    }

    void Renderer::DrawIndirect(uint numCommands, Buffer* indirectBuffer)
    {
        Instance->PlatformRenderer->DrawIndirect(numCommands, indirectBuffer);
    }

    void Renderer::SetIndexBuffer(Buffer* indexBuffer)
    {
        Instance->PlatformRenderer->SetIndexBuffer(indexBuffer);
    }

    void Renderer::SetVertexBuffers(Buffer* vertexBuffer, uint32 numBuffers, uint32 startIndex)
    {
        Instance->PlatformRenderer->SetVertexBuffers(vertexBuffer, numBuffers, startIndex);
    }

    void Renderer::Signal()
    {
        Instance->PlatformRenderer->Signal();
    }

    void Renderer::Wait()
    {
        Instance->PlatformRenderer->Wait();
    }

    void Renderer::Compute(Point3 groupCount)
    {
        Instance->PlatformRenderer->Compute(groupCount);
    }

    void Renderer::TraceRays(Pipeline* rayTracingPipeline, Point3 numRays)
    {
        Instance->PlatformRenderer->TraceRays(rayTracingPipeline, numRays);
    }

    PixelShader* Renderer::LoadPixelShader(const Path& shaderName, WString entryPoint)
    {
        return Instance->PlatformRenderer->LoadPixelShader(shaderName, entryPoint);
    }

    ComputeShader* Renderer::LoadComputeShader(const Path& shaderName, WString entryPoint)
    {
        return Instance->PlatformRenderer->LoadComputeShader(shaderName, entryPoint);
    }

    RayTracingShader* Renderer::LoadRayTracingShader(const Path& shaderName)
    {
        return Instance->PlatformRenderer->LoadRayTracingShader(shaderName);
    }

    void Renderer::SetPipeline(Pipeline* pipeline)
    {
        Instance->PlatformRenderer->SetPipeline(pipeline);
    }

    void Renderer::PushConstants(void* data, size_t size)
    {
        Instance->PlatformRenderer->PushConstants(data, size);
    }

    void Renderer::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil)
    {
        Instance->PlatformRenderer->SetRenderTargets(renderTargets, depthStencil);
    }

    Pipeline* Renderer::CreateGraphicPipeline(const WString& name,
                                                PixelShader* shader,
                                                WArray<TextureFormat> RTFormats = { TextureFormat::R8G8B8A8_UNORM },
                                                RasterizerDesc rasterizerDesc = DEFAULT_RASTERIZER_DESC,
                                                DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC,
                                                PrimitiveTopologyType primitiveTopologyType = WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                const WArray<InputLayoutDesc>& inputLayout = DEFAULT_INPUT_LAYOUT_DESC)
    {
        return Instance->PlatformRenderer->CreateGraphicPipeline(name, shader, RTFormats, rasterizerDesc, depthStencilDesc, primitiveTopologyType, inputLayout);
    }

    Pipeline* Renderer::CreateComputePipeline(const WString& name, ComputeShader* shader)
    {
        return Instance->PlatformRenderer->CreateComputePipeline(name, shader);
    }

    Pipeline* Renderer::CreateRayTracingPipeline(const WString& name, RayTracingShader* shader)
    {
        return Instance->PlatformRenderer->CreateRayTracingPipeline(name, shader);
    }

    Texture2D* Renderer::CreateTexture(WString name, int width, int height, TextureFormat format, uint8_t* data)
    {
        Texture2D* texture = Instance->PlatformRenderer->CreateTexture(name, width, height, format, data);
        return texture;
    }

    RenderTarget* Renderer::CreateRenderTarget(WString name, int width, int height, TextureFormat format)
    {
        RenderTarget* renderTarget = Instance->PlatformRenderer->CreateRenderTarget(name, width, height, format);
        return renderTarget;
    }

    SViewport* Renderer::GetEditorViewport()
    {
        return Instance->PlatformRenderer->GetEditorViewport();
    }

    SViewport* Renderer::GetGameViewport()
    {
        return Instance->PlatformRenderer->GetGameViewport();
    }

    SViewport* Renderer::GetMainViewport()
    {
        return Instance->PlatformRenderer->GetMainViewport();
    }

    AccelerationStructure* Renderer::CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries)
    {
        return Instance->PlatformRenderer->CreateBLAS(name, geometries);
    }

    AccelerationStructure* Renderer::CreateTLAS(WString name, WArray<RayTracingInstance>& instances)
    {
        return Instance->PlatformRenderer->CreateTLAS(name, instances);
    }

    void Renderer::UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries)
    {
        Instance->PlatformRenderer->UpdateBLAS(BLAS, geometries);
    }

    void Renderer::UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances)
    {
        Instance->PlatformRenderer->UpdateTLAS(TLAS, instances);
    }

    void Renderer::CopyResource(GraphicResource* dstResource, GraphicResource* srcResource)
    {
        Instance->PlatformRenderer->CopyResource(dstResource, srcResource);
    }

    Buffer* Renderer::CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride)
    {
        return Instance->PlatformRenderer->CreateBuffer(name, type, data, size, stride);
    }

    void Renderer::UpdateGraphicResource(GraphicResource* graphicResource, void* data, uint32_t size)
    {
        Instance->PlatformRenderer->UpdateGraphicResource(graphicResource, data, size);
    }

    void Renderer::ReadbackBuffer(Buffer* buffer, void* data)
    {
        Instance->PlatformRenderer->ReadbackBuffer(buffer, data);
    }

    void Renderer::ResourceBarrier(GraphicResource* resource, ResourceStates before, ResourceStates after)
    {
        Instance->PlatformRenderer->ResourceBarrier(resource, before, after);
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

    void Renderer::Destroy(GraphicResource* resource)
    {
        Instance->PlatformRenderer->Destroy(resource);
    }

    void* Renderer::GetPlatformResource(GraphicResource* resource)
    {
        return Instance->PlatformRenderer->GetPlatformResource(resource);
    }
}
