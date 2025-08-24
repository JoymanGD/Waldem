#pragma once
#include "Widget.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API IWidgetContainer : public IWidget
    {
    protected:
        WArray<IWidget*> Children;
    public:
        IWidgetContainer() {}
        IWidgetContainer(WArray<IWidget*> children) : Children(children) {}

        void AddChild(IWidget* child) { Children.Add(child); }
        void RemoveChild(IWidget* child) { Children.Remove(child); }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
            for(auto child : Children)
            {
                child->Initialize(inputManager, resourceManager);
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
