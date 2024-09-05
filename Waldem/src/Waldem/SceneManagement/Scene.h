#pragma once
#include "glm/vec4.hpp"

namespace Waldem
{
    class WALDEM_API Scene
    {
    public:
        Scene() {}
        virtual ~Scene() = default;
        virtual void Initialize() = 0;
        void DrawInternal();
        void UpdateInternal(float deltaTime);
        void SetClearColor(glm::vec4 Color);

        //Draw events
        std::function<void()> PreDraw = {};
        std::function<void()> PostDraw = {};

    protected:
        virtual void Clear();
        virtual void Draw() = 0;
        virtual void Update(float deltaTime) = 0;
        
        glm::vec4 ClearColor = { 0.f, 0.f, 0.f, 1.f };
    };
}