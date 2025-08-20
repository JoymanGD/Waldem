#pragma once

#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/SceneManagement/Scene.h"

namespace Waldem
{
    class WALDEM_API ISystem
    {
    protected:
        bool IsInitialized = false;
    public:
        ISystem() {}
        virtual void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) {}
        virtual void Deinitialize() {}
        virtual void Update(float deltaTime) {}
        virtual void OnResize(Vector2 size) {}
    };
}
