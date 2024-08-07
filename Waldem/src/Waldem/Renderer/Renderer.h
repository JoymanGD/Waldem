#pragma once

namespace Waldem
{
    enum class RendererAPI
    {
        None = 0,
        OpenGL = 1,
        DirectX = 2,
        Vulkan = 3,
    };

    class Renderer
    {
    public:
        inline static RendererAPI GetAPI() { return RAPI; }
        static RendererAPI RAPI;
    };
}