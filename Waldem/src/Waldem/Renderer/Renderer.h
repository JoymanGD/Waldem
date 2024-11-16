#pragma once
#include "RenderTarget.h"
#include "Model/Mesh.h"
#include "Model/Model.h"
#include "Waldem/Window.h"
#include "Resource.h"

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
        virtual void Draw(Model* model, PixelShader* pixelShader) = 0;
        virtual void Draw(Mesh* mesh, PixelShader* pixelShader) = 0;
        virtual Point3 GetNumThreadsPerGroup(ComputeShader* computeShader) = 0;
        virtual void Compute(ComputeShader* computeShader, Point3 groupCount) = 0;
        virtual PixelShader* LoadPixelShader(String shaderName, std::vector<Resource> resources, RenderTarget* renderTarget = nullptr) = 0;
        virtual ComputeShader* LoadComputeShader(String shaderName, std::vector<Resource> resources) = 0;
        virtual Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr) = 0;
        virtual RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format) = 0;
        virtual VertexBuffer* CreateVertexBuffer(void* data, uint32_t size) = 0;
        virtual IndexBuffer* CreateIndexBuffer(void* data, uint32_t count) = 0;
    };

    class Renderer
    {
    public:
        Renderer() = default;

        void Initialize(Window* window);

        void Begin();
        void End();
        void Present();

        void Draw(Mesh* mesh, PixelShader* pixelShader);
        void Draw(Model* model, PixelShader* pixelShader);
        Point3 GetNumThreadsPerGroup(ComputeShader* computeShader);
        void Compute(ComputeShader* computeShader, Point3 groupCount);
        PixelShader* LoadPixelShader(String shaderName, std::vector<Resource> resources, RenderTarget* renderTarget = nullptr);
        ComputeShader* LoadComputeShader(String shaderName, std::vector<Resource> resources);
        Texture2D* CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data = nullptr);
        RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format);
        VertexBuffer* CreateVertexBuffer(void* data, uint32_t size);
        IndexBuffer* CreateIndexBuffer(void* data, uint32_t count);

        static RendererAPI RAPI;
        
    private:
        IRenderer* PlatformRenderer;
    };
}
