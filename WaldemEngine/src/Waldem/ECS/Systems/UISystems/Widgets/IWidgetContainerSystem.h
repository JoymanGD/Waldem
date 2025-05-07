#pragma once
#include "WidgetSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API IWidgetContainerSystem : public IWidgetSystem
    {
    protected:
        WArray<IWidgetSystem*> Children;
    public:
        IWidgetContainerSystem(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}
        IWidgetContainerSystem(ECSManager* eCSManager, WArray<IWidgetSystem*> children) : IWidgetSystem(eCSManager), Children(children) {}

        void AddChild(IWidgetSystem* child) { Children.Add(child); }
        void RemoveChild(IWidgetSystem* child) { Children.Remove(child); }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
            for(auto child : Children)
            {
                child->Initialize(inputManager, resourceManager);
            }
        }

        void Update(float deltaTime) override
        {
            for(auto child : Children)
            {
                child->Update(deltaTime);
            }
        }
    };
}
