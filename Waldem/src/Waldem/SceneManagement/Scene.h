#pragma once
#include "glm/vec4.hpp"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    struct SceneData
    {
        Renderer* Renderer;
        Window* Window;
    };
    
    class WALDEM_API Scene
    {
    public:
        Scene() {}
        virtual ~Scene() = default;
        virtual void Initialize(SceneData* sceneData) = 0;
        void DrawInternal(SceneData* sceneData, float deltaTime);
        void UpdateInternal(SceneData* sceneData, float deltaTime);

        //Draw events
        std::function<void()> PreDraw = {};
        std::function<void()> PostDraw = {};

    protected:
        virtual void Draw(SceneData* sceneData, float deltaTime) = 0;
        virtual void Update(SceneData* sceneData, float deltaTime) = 0;
    };
}
