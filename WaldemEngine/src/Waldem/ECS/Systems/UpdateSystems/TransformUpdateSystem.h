#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API TransformUpdateSystem : public ISystem
    {
    public:
        TransformUpdateSystem() {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](Transform& transform)
            {
            });
        }
    };
}