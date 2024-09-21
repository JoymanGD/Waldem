#pragma once
#include "glm/vec4.hpp"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API Scene
    {
    public:
        Scene() {}
        virtual ~Scene() = default;
        virtual void Initialize() = 0;
        void DrawInternal(Renderer* renderer);
        void UpdateInternal(float deltaTime);

        //Draw events
        std::function<void()> PreDraw = {};
        std::function<void()> PostDraw = {};

    protected:
        virtual void Draw(Renderer* renderer) = 0;
        virtual void Update(float deltaTime) = 0;
    };
}
