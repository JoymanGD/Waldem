#pragma once
#include "Waldem/Renderer/Renderer.h"

class OpenGLRenderer : Waldem::IRenderer
{
public:
    void Initialize() override;
    void Clear(glm::vec4 clearColor) override;
    void Render(uint32_t indexCount) override;
};
