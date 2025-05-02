#pragma once
#include "ComponentWidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    template<typename T>
    class WALDEM_API ComponentWidget : public IComponentWidgetSystem
    {
    public:
        ComponentWidget(ECSManager* eCSManager) : IComponentWidgetSystem(eCSManager) {}

        bool IsVisible() override { return Manager->EntitiesWith<T, Selected>().Count() > 0; }
        
        void RemoveComponent(ecs::Entity& entity) override
        {
            entity.Remove<T>();
        }
        
        void ResetComponent(ecs::Entity& entity) override
        {
            if(entity.Has<T>())
            {
                IComponentBase& component = entity.Get<T>();
                component.Reset();
            }
        }
    };
}
