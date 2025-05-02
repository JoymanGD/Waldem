#pragma once
#include "Waldem/ECS/Systems/EditorSystems/Widgets/WidgetSystem.h"

namespace Waldem
{
    class WALDEM_API IComponentWidgetSystem : public IWidgetSystem
    {
    public:
        IComponentWidgetSystem(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        virtual void RemoveComponent(ecs::Entity& entity) = 0;
        virtual void ResetComponent(ecs::Entity& entity) = 0;
        virtual bool IsVisible() { return true; }
        virtual bool IsRemovable() { return true; }
        virtual bool IsResettable() { return true; }
    };
}
