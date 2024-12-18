#pragma once
#include "glm/vec4.hpp"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    struct SceneData
    {
        Window* Window;
    };
    
    class WALDEM_API Scene
    {
    public:
        Scene() {}
        virtual ~Scene() = default;
        virtual void Initialize(SceneData* sceneData, InputManager* inputManager) = 0;
        virtual void Draw(float deltaTime) = 0;
        virtual void Update(float deltaTime) = 0;
        virtual void DrawUI(float deltaTime) = 0;
    };
}
