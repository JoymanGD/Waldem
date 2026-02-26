#pragma once
#include "Waldem/Input/InputManager.h"
#include "Waldem/Types/MathTypes.h"

namespace Waldem
{
    class IWidget
    {
    public:
        IWidget() {}
        virtual void Initialize(InputManager* inputManager) {}
        virtual void Deinitialize() {}
        virtual void OnDraw(float deltaTime) = 0;
        virtual void OnResize(Vector2 size) {}
    };
}
