#include "wdpch.h"
#include "InputBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/Input/Input.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem::Bindings
{
    namespace
    {
        bool Input_IsKeyPressed(int keyCode)
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return false;
            return Input::IsKeyPressed(keyCode);
        }

        bool Input_IsMouseButtonPressed(int button)
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return false;
            return Input::IsMouseButtonPressed(button);
        }
    }

    void RegisterInputCalls(Mono* runtime)
    {
        BIND(runtime, Input_IsKeyPressed);
        BIND(runtime, Input_IsMouseButtonPressed);
    }
}
