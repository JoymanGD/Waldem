#pragma once

#include <ecs.h>
#include "Waldem/Renderer/Resources/ResourceManager.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/ECSManager.h"

namespace Waldem
{
    class WALDEM_API ISystem
    {
    protected:
        ECSManager* Manager = nullptr;
    public:
        ISystem(ECSManager* manager) : Manager(manager) {}
        virtual void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) = 0;
        virtual void Update(float deltaTime) = 0;
    };
}
