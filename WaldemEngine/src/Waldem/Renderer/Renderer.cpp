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

    void Renderer::DrawIndexedInstanced(uint indexCount, uint instanceCount, uint startIndexLocation, int baseVertexLocation, uint startInstanceLocation)
    {
        Instance->PlatformRenderer->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
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

    void Renderer::BindRenderTargets(RenderTarget* renderTarget)
    {
        Instance->PlatformRenderer->BindRenderTargets({ renderTarget });
    }

    void Renderer::BindRenderTargets(WArray<RenderTarget*> renderTargets)
    {
        Instance->PlatformRenderer->BindRenderTargets(renderTargets);
    }

    void Renderer::BindDepthStencil(RenderTarget* depthStencil)
    {
        Instance->PlatformRenderer->BindDepthStencil(depthStencil);
    }

    void Renderer::SetViewport(SViewport& viewport)
    {
        Instance->PlatformRenderer->SetViewport(viewport);
    }

    Pipeline* Renderer::CreateGraphicPipeline(const WString& name,
                                              PixelShader* shader,
                                              WArray<TextureFormat> RTFormats = { TextureFormat::R8G8B8A8_UNORM },
                                              TextureFormat depthFormat,
                                              RasterizerDesc rasterizerDesc,
                                              DepthStencilDesc depthStencilDesc,
                                              PrimitiveTopologyType primitiveTopologyType,
                                              const WArray<InputLayoutDesc>& inputLayout)
    {
        return Instance->PlatformRenderer->CreateGraphicPipeline(name, shader, RTFormats, depthFormat, rasterizerDesc, depthStencilDesc, primitiveTopologyType, inputLayout);
    }

    Pipeline* Renderer::CreateComputePipeline(const WString& name, ComputeShader* shader)
    {
        return Instance->PlatformRenderer->CreateComputePipeline(name, shader);
    }

    Pipeline* Renderer::CreateRayTracingPipeline(const WString& name, RayTracingShader* shader)
    {
        return Instance->PlatformRenderer->CreateRayTracingPipeline(name, shader);
    }

    Texture2D* Renderer::CreateTexture2D(WString name, int width, int height, TextureFormat format, uint8_t* data)
    {
        Texture2D* texture = Instance->PlatformRenderer->CreateTexture2D(name, width, height, format, data);
        return texture;
    }

    Texture3D* Renderer::CreateTexture3D(WString name, int width, int height, int depth, TextureFormat format, uint8_t* data)
    {
        return Instance->PlatformRenderer->CreateTexture3D(name, width, height, depth, format, data);
    }

    RenderTarget* Renderer::CreateRenderTarget(WString name, int width, int height, TextureFormat format)
    {
        RenderTarget* renderTarget = Instance->PlatformRenderer->CreateRenderTarget(name, width, height, format);

        if(RenderTargets.Contains(name))
        {
            RenderTargets[name] = renderTarget;
        }
        else
        {
            RenderTargets.Add(name, renderTarget);
        }
        
        return renderTarget;
    }

    void Renderer::InitializeRenderTarget(WString name, int width, int height, TextureFormat format, RenderTarget*& renderTarget)
    {
        Instance->PlatformRenderer->InitializeRenderTarget(name, width, height, format, renderTarget);
    }

    RenderTarget* Renderer::ResizeRenderTarget(WString name, int width, int height)
    {
        auto& renderTarget = RenderTargets[name];

        if(renderTarget)
        {
            auto format = renderTarget->GetFormat();
            
            Destroy(renderTarget);
            
            InitializeRenderTarget(name, width, height, format, renderTarget);
        }

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

    AccelerationStructure* Renderer::CreateTLAS(WString name, Buffer* instanceBuffer, uint numInstances)
    {
        return Instance->PlatformRenderer->CreateTLAS(name, instanceBuffer, numInstances);
    }

    void Renderer::InitializeTLAS(WString name, Buffer* instanceBuffer, uint numInstances, AccelerationStructure*& tlas)
    {
        Instance->PlatformRenderer->InitializeTLAS(name, instanceBuffer, numInstances, tlas);
    }

    void Renderer::BuildTLAS(Buffer* instanceBuffer, uint numInstances, AccelerationStructure*& tlas)
    {
        Instance->PlatformRenderer->BuildTLAS(instanceBuffer, numInstances, tlas);
    }

    void Renderer::UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries)
    {
        Instance->PlatformRenderer->UpdateBLAS(BLAS, geometries);
    }

    void Renderer::UpdateTLAS(AccelerationStructure* TLAS, Buffer* instanceBuffer, uint numInstances)
    {
        Instance->PlatformRenderer->UpdateTLAS(TLAS, instanceBuffer, numInstances);
    }

    void Renderer::CopyResource(GraphicResource* dstResource, GraphicResource* srcResource)
    {
        Instance->PlatformRenderer->CopyResource(dstResource, srcResource);
    }

    void Renderer::CopyBufferRegion(GraphicResource* dstResource, size_t dstOffset, GraphicResource* srcResource, size_t srcOffset, size_t size)
    {
        Instance->PlatformRenderer->CopyBufferRegion(dstResource, dstOffset, srcResource, srcOffset, size);
    }

    Buffer* Renderer::CreateBuffer(WString name, BufferType type, uint32_t size, uint32_t stride, void* data, size_t dataSize)
    {
        return Instance->PlatformRenderer->CreateBuffer(name, type, size, stride, data, dataSize);
    }

    void Renderer::InitializeBuffer(WString name, BufferType type, uint32_t size, uint32_t stride, Buffer*& buffer, void* data, size_t dataSize)
    {
        Instance->PlatformRenderer->InitializeBuffer(name, type, size, stride, buffer, data, dataSize);
    }

    void Renderer::UploadBuffer(Buffer* buffer, void* data, uint32_t size, uint offset)
    {
        Instance->PlatformRenderer->UploadBuffer(buffer, data, size, offset);
    }

    void Renderer::ClearBuffer(Buffer* buffer, uint32_t size, uint offset)
    {
        Instance->PlatformRenderer->ClearBuffer(buffer, size, offset);
    }

    void Renderer::DownloadBuffer(Buffer* buffer, void* data, size_t size)
    {
        Instance->PlatformRenderer->DownloadBuffer(buffer, data, size);
    }

    void Renderer::ResourceBarrier(GraphicResource* resource, ResourceStates before, ResourceStates after)
    {
        Instance->PlatformRenderer->ResourceBarrier(resource, before, after);
    }

    ResourceStates Renderer::ResourceBarrier(GraphicResource* resource, ResourceStates after)
    {
        return Instance->PlatformRenderer->ResourceBarrier(resource, after);
    }

    void Renderer::UAVBarrier(GraphicResource* resource)
    {
        Instance->PlatformRenderer->UAVBarrier(resource);
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

    void Renderer::DestroyImmediate(GraphicResource* resource)
    {
        Instance->PlatformRenderer->DestroyImmediate(resource);
    }

    void* Renderer::GetPlatformResource(GraphicResource* resource)
    {
        return Instance->PlatformRenderer->GetPlatformResource(resource);
    }
}
