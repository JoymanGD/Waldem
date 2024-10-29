#pragma once
#include "Model/Mesh.h"
#include "Model/Model.h"
#include "Waldem/Window.h"

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
        virtual void SetFrameIndex(uint32_t frame) = 0;
        virtual void DrawMesh(Mesh* mesh, PixelShader* pixelShader) = 0;
        virtual PixelShader* LoadShader(std::string shaderName) = 0;
    };

    class Renderer
    {
    public:
        Renderer() = default;

        void Initialize(Window* window);

        void Begin(uint32_t frame);
        void End();

        void DrawMesh(Mesh* mesh, PixelShader* pixelShader);
        void DrawModel(Model* model, PixelShader* pixelShader);
        PixelShader* LoadShader(std::string shaderName);

        static RendererAPI RAPI;
        
    private:
        IRenderer* CurrentRenderer;
    };
}
