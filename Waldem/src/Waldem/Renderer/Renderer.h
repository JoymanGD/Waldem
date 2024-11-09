#pragma once
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
        virtual void Draw(Mesh* mesh, PixelShader* pixelShader) = 0;
        virtual PixelShader* LoadShader(std::string shaderName, std::vector<Resource> resources) = 0;
        virtual Texture2D* CreateTexture(std::string name, int width, int height, int channels, uint8_t* data = nullptr) = 0;
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
        PixelShader* LoadShader(std::string shaderName, std::vector<Resource> resources);
        Texture2D* CreateTexture(std::string name, int width, int height, int channels, uint8_t* data = nullptr);
        VertexBuffer* CreateVertexBuffer(void* data, uint32_t size);
        IndexBuffer* CreateIndexBuffer(void* data, uint32_t count);

        static RendererAPI RAPI;
        
    private:
        IRenderer* PlatformRenderer;
    };
}
