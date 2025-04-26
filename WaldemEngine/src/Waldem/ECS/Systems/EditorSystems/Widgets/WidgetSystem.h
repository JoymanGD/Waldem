#pragma once
#include "imgui.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API IWidgetSystem : public ISystem
    {
    public:
        IWidgetSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        virtual WString GetName() = 0;
        virtual bool IsVisible() { return true; }
        virtual bool IsRemovable() { return true; }
        virtual bool IsResettable() { return true; }
    };
}
