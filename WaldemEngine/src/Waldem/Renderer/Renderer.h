#pragma once
#include "AccelerationStructure.h"
#include "CommandSignature.h"
#include "Pipeline.h"
#include "Waldem/Window.h"
#include "GraphicResource.h"
#include "Shader.h"
#include "Texture.h"

#define SWAPCHAIN_SIZE 2

namespace Waldem
{
    class CMesh;
    class RenderTarget;
    struct SViewport;
    struct CModel;

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
        virtual void Initialize(CWindow* window) = 0;
        virtual void Begin() = 0; 
        virtual void End() = 0;
        virtual void Present() = 0;
        virtual void Draw(CMesh* mesh) = 0;
        virtual void DrawIndirect(uint numCommands, Buffer* indirectBuffer) = 0;
        virtual void SetIndexBuffer(Buffer* indexBuffer) = 0;
        virtual void SetVertexBuffers(Buffer* vertexBuffer, uint32 numBuffers, uint32 startIndex = 0) = 0;
        virtual void Signal() = 0;
        virtual void Wait() = 0;
        virtual Point3 GetNumThreadsPerGroup(ComputeShader* computeShader) = 0;
        virtual void Compute(Point3 groupCount) = 0;
        virtual void TraceRays(Pipeline* rayTracingPipeline, Point3 numRays) = 0;
        virtual PixelShader* LoadPixelShader(const Path& shaderName, WString entryPoint) = 0;
        virtual ComputeShader* LoadComputeShader(const Path& shaderName, WString entryPoint) = 0;
        virtual RayTracingShader* LoadRayTracingShader(const Path& shaderName) = 0;
        virtual void SetPipeline(Pipeline* pipeline) = 0;
        virtual void PushConstants(void* data, size_t size) = 0;
        virtual void BindRenderTargets(WArray<RenderTarget*> renderTargets = {}) = 0;
        virtual void BindDepthStencil(RenderTarget* depthStencil = nullptr) = 0;
        virtual void SetViewport(SViewport& viewport) = 0;
        virtual Pipeline* CreateGraphicPipeline(const WString& name, PixelShader* shader, WArray<TextureFormat> RTFormats, TextureFormat depthFormat, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout) = 0;
        virtual Pipeline* CreateComputePipeline(const WString& name, ComputeShader* shader) = 0;
        virtual Pipeline* CreateRayTracingPipeline(const WString& name, RayTracingShader* shader) = 0;
        virtual Texture2D* CreateTexture(WString name, int width, int height, TextureFormat format, uint8_t* data = nullptr) = 0;
        virtual RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format) = 0;
        virtual SViewport* GetEditorViewport() = 0;
        virtual SViewport* GetGameViewport() = 0;
        virtual SViewport* GetMainViewport() = 0;
        virtual AccelerationStructure* CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries) = 0;
        virtual AccelerationStructure* CreateTLAS(WString name, WArray<RayTracingInstance>& instances) = 0;
        virtual void UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries) = 0;
        virtual void UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances) = 0;
        virtual void CopyResource(GraphicResource* dstResource, GraphicResource* srcResource) = 0;
        virtual Buffer* CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride) = 0;
        virtual void ResourceBarrier(GraphicResource* resource, ResourceStates before, ResourceStates after) = 0;
        virtual void UploadBuffer(Buffer* buffer, void* data, uint32_t size, uint offset = 0) = 0;
        virtual void DownloadBuffer(Buffer* buffer, void* data) = 0;
        virtual void ClearRenderTarget(RenderTarget* rt) = 0;
        virtual void ClearDepthStencil(RenderTarget* ds) = 0;
        virtual void InitializeUI() = 0;
        virtual void DeinitializeUI() = 0;
        virtual void BeginUI() = 0;
        virtual void EndUI() = 0;
        virtual void Destroy(GraphicResource* resource) = 0;
        virtual void* GetPlatformResource(GraphicResource* resource) = 0;
    };

    class Renderer
    {
    public:
        Renderer() = default;

        void Initialize(CWindow* window);

        static void Begin();
        static void End();
        static void Present();

        static void Draw(CMesh* mesh);
        static void DrawIndirect(uint numCommands, Buffer* indirectBuffer);
        static void SetIndexBuffer(Buffer* indexBuffer);
        static void SetVertexBuffers(Buffer* vertexBuffer, uint32 numBuffers, uint32 startIndex = 0);
        static void Signal();
        static void Wait();
        static Point3 GetNumThreadsPerGroup(ComputeShader* computeShader);
        static void Compute(Point3 groupCount);
        static void TraceRays(Pipeline* rayTracingPipeline, Point3 numRays);
        static PixelShader* LoadPixelShader(const Path& shaderName, WString entryPoint = "main");
        static ComputeShader* LoadComputeShader(const Path& shaderName, WString entryPoint = "main");
        static RayTracingShader* LoadRayTracingShader(const Path& shaderName);
        static void SetPipeline(Pipeline* pipeline);
        static void PushConstants(void* data, size_t size);
        static void BindRenderTargets(RenderTarget* renderTarget = nullptr);
        static void BindRenderTargets(WArray<RenderTarget*> renderTargets = {});
        static void BindDepthStencil(RenderTarget* depthStencil = nullptr);
        static void SetViewport(SViewport& viewport);
        static Pipeline* CreateGraphicPipeline(const WString& name, PixelShader* shader, WArray<TextureFormat> RTFormats, TextureFormat depthFormat = TextureFormat::D32_FLOAT, RasterizerDesc rasterizerDesc = DEFAULT_RASTERIZER_DESC, DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC, PrimitiveTopologyType primitiveTopologyType = WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, const WArray<InputLayoutDesc>& inputLayout = DEFAULT_INPUT_LAYOUT_DESC);
        static Pipeline* CreateComputePipeline(const WString& name, ComputeShader* shader);
        static Pipeline* CreateRayTracingPipeline(const WString& name, RayTracingShader* shader);
        static Texture2D* CreateTexture(WString name, int width, int height, TextureFormat format, uint8_t* data = nullptr);
        static RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format);
        static SViewport* GetEditorViewport();
        static SViewport* GetGameViewport();
        static SViewport* GetMainViewport();
        static AccelerationStructure* CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries);
        static AccelerationStructure* CreateTLAS(WString name, WArray<RayTracingInstance>& instances);
        static void UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries);
        static void UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances);
        static void CopyResource(GraphicResource* dstResource, GraphicResource* srcResource);
        static Buffer* CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride);
        static void ResourceBarrier(GraphicResource* resource, ResourceStates before, ResourceStates after);
        static void UploadBuffer(Buffer* buffer, void* data, uint32_t size, uint offset = 0);
        static void DownloadBuffer(Buffer* buffer, void* data);
        static void ClearRenderTarget(RenderTarget* rt);
        static void ClearDepthStencil(RenderTarget* ds);
        static void InitializeUI();
        static void BeginUI();
        static void EndUI();
        static void Destroy(GraphicResource* resource);
        static void* GetPlatformResource(GraphicResource* resource);

        static RendererAPI RAPI;
        inline static Renderer* Instance;
        
    private:
        IRenderer* PlatformRenderer;
    };
}
