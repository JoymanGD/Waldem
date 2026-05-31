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
        bool Input_GetKey(int keyCode)
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return false;
            return Input::GetKey(keyCode);
        }

        bool Input_GetKeyDown(int keyCode)
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return false;
            return Input::GetKeyDown(keyCode);
        }

        bool Input_GetMouse(int button)
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return false;
            return Input::GetMouse(button);
        }

        bool Input_GetMouseDown(int button)
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return false;
            return Input::GetMouseDown(button);
        }

        float Input_GetMouseDeltaX()
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return 0.0f;
            return static_cast<float>(Input::GetMouseDelta().x);
        }

        float Input_GetMouseDeltaY()
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport != nullptr && !gameViewport->IsFocused)
                return 0.0f;
            return static_cast<float>(Input::GetMouseDelta().y);
        }
    }

    void RegisterInputCalls(Mono* runtime)
    {
        BIND(runtime, Input_GetKey);
        BIND(runtime, Input_GetKeyDown);
        BIND(runtime, Input_GetMouse);
        BIND(runtime, Input_GetMouseDown);
        BIND(runtime, Input_GetMouseDeltaX);
        BIND(runtime, Input_GetMouseDeltaY);
    }
}
