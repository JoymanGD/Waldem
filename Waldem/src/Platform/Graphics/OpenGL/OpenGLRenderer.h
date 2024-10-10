#pragma once
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class OpenGLRenderer : public IRenderer
    {
    public:
        void Initialize(Window* window) override;
        void Clear(Vector4 clearColor) override;
        void Render(uint32_t indexCount) override;
    };
}