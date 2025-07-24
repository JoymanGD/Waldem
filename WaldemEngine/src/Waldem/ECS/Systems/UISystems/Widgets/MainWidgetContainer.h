#pragma once
#include "WidgetContainerSystem.h"

namespace Waldem
{
    class WALDEM_API MainWidgetContainer : public IWidgetContainerSystem
    {
    public:
        MainWidgetContainer(WArray<IWidgetSystem*> children) : IWidgetContainerSystem(children) {} 

        WString GetName() override { return "MainWidgetContainer"; } 
    };
}
