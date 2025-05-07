#pragma once

#include <ecs.h>
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/ECSManager.h"

namespace Waldem
{
    class WALDEM_API ISystem
    {
    protected:
        ECSManager* Manager = nullptr;
        bool IsInitialized = false;
    public:
        ISystem(ECSManager* manager) : Manager(manager) {}
        virtual void Initialize(InputManager* inputManager, ResourceManager* resourceManager) = 0;
        virtual void Deinitialize() {}
        virtual void Update(float deltaTime) = 0;
    };
}
