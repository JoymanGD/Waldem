#pragma once
#include "imgui.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API IWidget
    {
    public:
        IWidget() {}
        virtual void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) {}
        virtual void Deinitialize() {}
        virtual WString GetName() = 0;
        virtual void OnDraw(float deltaTime) {}
    };
}
