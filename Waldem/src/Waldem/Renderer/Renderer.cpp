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

    void Renderer::Draw(CMesh* mesh)
    {
        Instance->PlatformRenderer->Draw(mesh);
    }

    void Renderer::Draw(Model* model)
    {
        Instance->PlatformRenderer->Draw(model);
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

    PixelShader* Renderer::LoadPixelShader(String shaderName, String entryPoint)
    {
        return Instance->PlatformRenderer->LoadPixelShader(shaderName, entryPoint);
    }

    ComputeShader* Renderer::LoadComputeShader(String shaderName, String entryPoint)
    {
        return Instance->PlatformRenderer->LoadComputeShader(shaderName, entryPoint);
    }

    RayTracingShader* Renderer::LoadRayTracingShader(String shaderName)
    {
        return Instance->PlatformRenderer->LoadRayTracingShader(shaderName);
    }

    void Renderer::SetPipeline(Pipeline* pipeline)
    {
        Instance->PlatformRenderer->SetPipeline(pipeline);
    }

    void Renderer::SetRootSignature(RootSignature* rootSignature)
    {
        Instance->PlatformRenderer->SetRootSignature(rootSignature);
    }

    void Renderer::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil, SViewport viewport, SScissorRect scissor)
    {
        Instance->PlatformRenderer->SetRenderTargets(renderTargets, depthStencil, viewport, scissor);
    }

    Pipeline* Renderer::CreateGraphicPipeline(const String& name,
                                                RootSignature* rootSignature,
                                                PixelShader* shader,
                                                WArray<TextureFormat> RTFormats = { TextureFormat::R8G8B8A8_UNORM },
                                                RasterizerDesc rasterizerDesc = DEFAULT_RASTERIZER_DESC,
                                                DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC,
                                                PrimitiveTopologyType primitiveTopologyType = WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                const WArray<InputLayoutDesc>& inputLayout = DEFAULT_INPUT_LAYOUT_DESC)
    {
        return Instance->PlatformRenderer->CreateGraphicPipeline(name, rootSignature, shader, RTFormats, rasterizerDesc, depthStencilDesc, primitiveTopologyType, inputLayout);
    }

    Pipeline* Renderer::CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader)
    {
        return Instance->PlatformRenderer->CreateComputePipeline(name, rootSignature, shader);
    }

    Pipeline* Renderer::CreateRayTracingPipeline(const String& name, RootSignature* rootSignature, RayTracingShader* shader)
    {
        return Instance->PlatformRenderer->CreateRayTracingPipeline(name, rootSignature, shader);
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

    AccelerationStructure* Renderer::CreateBLAS(String name, WArray<RayTracingGeometry>& geometries)
    {
        return Instance->PlatformRenderer->CreateBLAS(name, geometries);
    }

    AccelerationStructure* Renderer::CreateTLAS(String name, WArray<RayTracingInstance>& instances)
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

    void Renderer::CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT)
    {
        Instance->PlatformRenderer->CopyRenderTarget(dstRT, srcRT);
    }

    void Renderer::CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer)
    {
        Instance->PlatformRenderer->CopyBuffer(dstBuffer, srcBuffer);
    }

    Buffer* Renderer::CreateBuffer(String name, BufferType type, void* data, uint32_t size, uint32_t stride)
    {
        return Instance->PlatformRenderer->CreateBuffer(name, type, data, size, stride);
    }

    void Renderer::UpdateBuffer(Buffer* buffer, void* data, uint32_t size)
    {
        Instance->PlatformRenderer->UpdateBuffer(buffer, data, size);
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