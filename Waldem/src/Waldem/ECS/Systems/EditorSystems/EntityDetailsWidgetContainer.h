#pragma once
#include "Waldem/ECS/Systems/EditorSystems/IWidgetContainerSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API EntityDetailsWidgetContainer : public IWidgetContainerSystem
    {
    public:
        EntityDetailsWidgetContainer(ecs::Manager* eCSManager, WArray<ISystem*> children) : IWidgetContainerSystem(eCSManager, children) {}

        void Update(float deltaTime) override
        {
            if(!ECSManager->EntitiesWith<Selected>().GetVector().empty())
            {
                for(auto child : Children)
                {
                    child->Update(deltaTime);
                }
            }
        }
    };
}
