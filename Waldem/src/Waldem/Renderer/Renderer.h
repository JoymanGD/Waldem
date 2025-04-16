#pragma once
#include "Pipeline.h"
#include "RenderTarget.h"
#include "Model/Mesh.h"
#include "Model/Model.h"
#include "Waldem/Window.h"
#include "Resource.h"
#include "RootSignature.h"

#define SWAPCHAIN_SIZE 2

namespace Waldem
{
    enum class RendererAPI
    {
        None = 0,
        DirectX = 1,
        Vulkan = 2,
    };

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;
        virtual void Initialize(Window* window) = 0;
        virtual void Begin() = 0; 
        virtual void End() = 0;
        virtual void Present() = 0;
        virtual void Draw(CModel* model) = 0;
        virtual void Draw(CMesh* mesh) = 0;
        virtual void Signal() = 0;
        virtual void Wait() = 0;
        virtual Point3 GetNumThreadsPerGroup(ComputeShader* computeShader) = 0;
        virtual void Compute(Point3 groupCount) = 0;
        virtual void TraceRays(Pipeline* rayTracingPipeline, Point3 numRays) = 0;
        virtual PixelShader* LoadPixelShader(String shaderName, String entryPoint) = 0;
        virtual ComputeShader* LoadComputeShader(String shaderName, String entryPoint) = 0;
        virtual RayTracingShader* LoadRayTracingShader(String shaderName) = 0;
        virtual void SetPipeline(Pipeline* pipeline) = 0;
        virtual void SetRootSignature(RootSignature* rootSignature) = 0;
        virtual void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr, SViewport viewport = {}, SScissorRect scissor = {}) = 0;
        virtual Pipeline* CreateGraphicPipeline(const String& name, RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout) = 0;
        virtual Pipeline* CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader) = 0;
        virtual Pipeline* CreateRayTracingPipeline(const String& name, RootSignature* rootSignature, RayTracingShader* shader) = 0;
        virtual RootSignature* CreateRootSignature(WArray<Resource> resources) = 0;
        virtual Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr) = 0;
        virtual RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format) = 0;
        virtual AccelerationStructure* CreateBLAS(String name, WArray<RayTracingGeometry>& geometries) = 0;
        virtual AccelerationStructure* CreateTLAS(String name, WArray<RayTracingInstance>& instances) = 0;
        virtual void UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries) = 0;
        virtual void UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances) = 0;
        virtual void CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT) = 0;
        virtual void CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer) = 0;
        virtual Buffer* CreateBuffer(String name, BufferType type, void* data, uint32_t size, uint32_t stride) = 0;
        virtual void ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after) = 0;
        virtual void ResourceBarrier(Buffer* buffer, ResourceStates before, ResourceStates after) = 0;
        virtual void UpdateBuffer(Buffer* buffer, void* data, uint32_t size) = 0;
        virtual void ClearRenderTarget(RenderTarget* rt) = 0;
        virtual void ClearDepthStencil(RenderTarget* ds) = 0;
        virtual void InitializeUI() = 0;
        virtual void BeginUI() = 0;
        virtual void EndUI() = 0;
    };

    class Renderer
    {
    public:
        Renderer() = default;

        void Initialize(Window* window);

        static void Begin();
        static void End();
        static void Present();

        static void Draw(CMesh* mesh);
        static void Draw(CModel* model);
        static void Signal();
        static void Wait();
        static Point3 GetNumThreadsPerGroup(ComputeShader* computeShader);
        static void Compute(Point3 groupCount);
        static void TraceRays(Pipeline* rayTracingPipeline, Point3 numRays);
        static PixelShader* LoadPixelShader(String shaderName, String entryPoint = "main");
        static ComputeShader* LoadComputeShader(String shaderName, String entryPoint = "main");
        static RayTracingShader* LoadRayTracingShader(String shaderName);
        static void SetPipeline(Pipeline* pipeline);
        static void SetRootSignature(RootSignature* rootSignature);
        static void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr, SViewport viewport = {}, SScissorRect scissor = {});
        static Pipeline* CreateGraphicPipeline(const String& name, RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout);
        static Pipeline* CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader);
        static Pipeline* CreateRayTracingPipeline(const String& name, RootSignature* rootSignature, RayTracingShader* shader);
        static RootSignature* CreateRootSignature(WArray<Resource> resources);
        static Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr);
        static RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format);
        static AccelerationStructure* CreateBLAS(String name, WArray<RayTracingGeometry>& geometries);
        static AccelerationStructure* CreateTLAS(String name, WArray<RayTracingInstance>& instances);
        static void UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries);
        static void UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances);
        static void CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT);
        static void CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer);
        static Buffer* CreateBuffer(String name, BufferType type, void* data, uint32_t size, uint32_t stride);
        static void ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after);
        static void ResourceBarrier(Buffer* buffer, ResourceStates before, ResourceStates after);
        static void UpdateBuffer(Buffer* buffer, void* data, uint32_t size);
        static void ClearRenderTarget(RenderTarget* rt);
        static void ClearDepthStencil(RenderTarget* ds);
        static void InitializeUI();
        static void BeginUI();
        static void EndUI();

        static RendererAPI RAPI;
        inline static Renderer* Instance;
        
    private:
        IRenderer* PlatformRenderer;
    };
}
