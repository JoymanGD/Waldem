#pragma once

#include "Waldem/Core.h"
#include "Waldem/Window.h"
#include "Waldem/Events/Event.h"
#include "Waldem/SceneManagement/SceneManager.h"

namespace Waldem
{
    class WALDEM_API Layer
    {
    public:
        Layer(const String& name = "Layer", Window* window = nullptr, ecs::Manager* ecsManager = nullptr, InputManager* inputManager = nullptr) : DebugName(name), MainWindow(window), CoreECSManager(ecsManager), CurrentInputManager(inputManager) {}
        virtual ~Layer() = default;
        virtual void Begin() {}
        virtual void End() {}
        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnDrawUI(float deltaTime) {}
        virtual void OnEvent(Event& event) {}

        inline const String& GetName() const { return DebugName; }
    protected:
        String DebugName;
        Window* MainWindow;
        ecs::Manager* CoreECSManager;
        InputManager* CurrentInputManager;
    };
}