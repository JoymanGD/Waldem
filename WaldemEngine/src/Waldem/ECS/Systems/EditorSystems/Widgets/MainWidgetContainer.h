#pragma once
#include "Waldem/ECS/Systems/EditorSystems/Widgets/IWidgetContainerSystem.h"

namespace Waldem
{
    class WALDEM_API MainWidgetContainer : public IWidgetContainerSystem
    {
    public:
        MainWidgetContainer(ECSManager* eCSManager, WArray<IWidgetSystem*> children) : IWidgetContainerSystem(eCSManager, children) {} 

        WString GetName() override { return "MainWidgetContainer"; } 
    };
}
