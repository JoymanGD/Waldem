#pragma once

#include "Waldem/ContentManagement/ContentManager.h"
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
    };
}
