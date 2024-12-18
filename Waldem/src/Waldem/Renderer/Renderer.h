#pragma once
#include "Line.h"
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
        virtual void Draw(Model* model) = 0;
        virtual void Draw(Mesh* mesh) = 0;
        virtual void DrawLine(Line line) = 0;
        virtual void DrawLines(WArray<Line> lines) = 0;
        virtual void Wait() = 0;
        virtual Point3 GetNumThreadsPerGroup(ComputeShader* computeShader) = 0;
        virtual void Compute(Point3 groupCount) = 0;
        virtual PixelShader* LoadPixelShader(String shaderName) = 0;
        virtual ComputeShader* LoadComputeShader(String shaderName) = 0;
        virtual void SetPipeline(Pipeline* pipeline) = 0;
        virtual void SetRootSignature(RootSignature* rootSignature) = 0;
        virtual void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr) = 0;
        virtual Pipeline* CreateGraphicPipeline(const String& name, WArray<TextureFormat> RTFormats, PrimitiveTopologyType primitiveTopologyType, RootSignature* rootSignature, PixelShader* shader) = 0;
        virtual Pipeline* CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader) = 0;
        virtual RootSignature* CreateRootSignature(WArray<Resource> resources) = 0;
        virtual Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr) = 0;
        virtual RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format) = 0;
        virtual VertexBuffer* CreateVertexBuffer(void* data, uint32_t size) = 0;
        virtual IndexBuffer* CreateIndexBuffer(void* data, uint32_t count) = 0;
        virtual void ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after) = 0;
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

        static void Draw(Mesh* mesh);
        static void Draw(Model* model);
        static void DrawLine(Line line);
        static void DrawLines(WArray<Line> lines);
        static void Wait();
        static Point3 GetNumThreadsPerGroup(ComputeShader* computeShader);
        static void Compute(Point3 groupCount);
        static PixelShader* LoadPixelShader(String shaderName);
        static ComputeShader* LoadComputeShader(String shaderName);
        static void SetPipeline(Pipeline* pipeline);
        static void SetRootSignature(RootSignature* rootSignature);
        static void SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil = nullptr);
        static Pipeline* CreateGraphicPipeline(const String& name, WArray<TextureFormat> RTFormats, PrimitiveTopologyType primitiveTopologyType, RootSignature* rootSignature, PixelShader* shader);
        static Pipeline* CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader);
        static RootSignature* CreateRootSignature(WArray<Resource> resources);
        static Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr);
        static RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format);
        static VertexBuffer* CreateVertexBuffer(void* data, uint32_t size);
        static IndexBuffer* CreateIndexBuffer(void* data, uint32_t count);
        static void ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after);
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
