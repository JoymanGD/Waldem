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
        IWidgetContainerSystem() {}
        IWidgetContainerSystem(WArray<IWidgetSystem*> children) : Children(children) {}

        void AddChild(IWidgetSystem* child) { Children.Add(child); }
        void RemoveChild(IWidgetSystem* child) { Children.Remove(child); }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            for(auto child : Children)
            {
                child->Initialize(inputManager, resourceManager, contentManager);
            }
        }

        void OnDraw(float deltaTime) override
        {
            for(auto child : Children)
            {
                child->OnDraw(deltaTime);
            }
        }
    };
}
