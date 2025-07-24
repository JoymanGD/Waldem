#pragma once

#include "Waldem/Input/InputManager.h"
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/Window.h"

namespace Waldem
{
    struct SceneData
    {
        CWindow* Window;
    };
    
    class WALDEM_API IScene
    {
    public:
        virtual void Initialize(InputManager* inputManager, ResourceManager* resourceManager) = 0;
        virtual void Draw(float deltaTime) = 0;
        virtual void Update(float deltaTime) = 0;
        virtual void FixedUpdate(float fixedDeltaTime) = 0;
        virtual void DrawUI(float deltaTime) = 0;
    };
}
