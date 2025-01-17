#pragma once
#include <ecs.h>

#include "Waldem/Renderer/Resources/ResourceManager.h"
#include "Waldem/SceneManagement/Scene.h"

namespace Waldem
{
    class WALDEM_API ISystem
    {
    protected:
        ecs::Manager* ECSManager = nullptr;
    public:
        ISystem(ecs::Manager* ECSManager) : ECSManager(ECSManager) {}
        virtual void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) = 0;
        virtual void Update(float deltaTime) = 0;
    };
}
