#pragma once

#include "Waldem/Input/InputManager.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"
#include "Waldem/Window.h"
#include "ecs.h"

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
        virtual void Initialize(SceneData* sceneData, InputManager* inputManager, ecs::Manager* ecsManager, ResourceManager* resourceManager) = 0;
        virtual void Draw(float deltaTime) = 0;
        virtual void Update(float deltaTime) = 0;
        virtual void DrawUI(float deltaTime) = 0;
    };
}
