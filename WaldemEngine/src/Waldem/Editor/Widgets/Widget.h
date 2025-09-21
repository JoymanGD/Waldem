#pragma once

namespace Waldem
{
    class WALDEM_API IWidget
    {
    public:
        IWidget() {}
        virtual void Initialize(InputManager* inputManager) {}
        virtual void Deinitialize() {}
        virtual void OnDraw(float deltaTime) = 0;
        virtual void OnResize(Vector2 size) {}
    };
}
