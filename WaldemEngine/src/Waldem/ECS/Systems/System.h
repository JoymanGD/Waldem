#pragma once

#include "Waldem/Input/InputManager.h"

namespace Waldem
{
    class WALDEM_API ISystem
    {
    protected:
        bool IsInitialized = false;
    public:
        ISystem() {}
        virtual void Initialize(InputManager* inputManager) {}
        virtual void Deinitialize() {}
        virtual void Update(float deltaTime) {}
        virtual void OnResize(Vector2 size) {}
    };
}
