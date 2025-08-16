#pragma once
#include "WidgetContainer.h"

namespace Waldem
{
    class WALDEM_API MainWidgetContainer : public IWidgetContainer
    {
    public:
        MainWidgetContainer(WArray<IWidget*> children) : IWidgetContainer(children) {} 

        WString GetName() override { return "MainWidgetContainer"; } 
    };
}
