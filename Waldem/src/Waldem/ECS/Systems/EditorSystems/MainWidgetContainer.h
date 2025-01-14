#pragma once
#include "Waldem/ECS/Systems/EditorSystems/IWidgetContainerSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API MainWidgetContainer : public IWidgetContainerSystem
    {
    public:
        MainWidgetContainer(ecs::Manager* eCSManager, WArray<ISystem*> children) : IWidgetContainerSystem(eCSManager, children) {}
    };
}
