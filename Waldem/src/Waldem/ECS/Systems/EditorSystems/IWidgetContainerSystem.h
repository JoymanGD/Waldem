#pragma once
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API IWidgetContainerSystem : public ISystem
    {
    protected:
        WArray<ISystem*> Children;
    public:
        IWidgetContainerSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        IWidgetContainerSystem(ecs::Manager* eCSManager, WArray<ISystem*> children) : ISystem(eCSManager), Children(children) {}

        void AddChild(ISystem* child) { Children.Add(child); }
        void RemoveChild(ISystem* child) { Children.Remove(child); }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            for(auto child : Children)
            {
                child->Initialize(sceneData, inputManager, resourceManager);
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
