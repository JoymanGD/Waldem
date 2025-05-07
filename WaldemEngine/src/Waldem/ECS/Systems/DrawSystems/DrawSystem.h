#pragma once
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API DrawSystem : public ISystem
    {
        
    public:
        DrawSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}

        virtual void OnResize(Vector2 size) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
            IsInitialized = true;
        }

        void Deinitialize() override
        {
            IsInitialized = false;
        }

        void Update(float deltaTime) override
        {
        }
    };
}
