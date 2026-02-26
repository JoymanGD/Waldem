#include "wdpch.h"
#include "WindowsInput.h"

#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Waldem/Engine.h"

namespace Waldem
{
    void Input::Initialize()
    {
        Instance = new WindowsInput();
    }
    
    bool WindowsInput::IsKeyPressedImpl(int keycode)
    {
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        
        return state[SDL_GetScancodeFromKey(keycode)] != 0;
    }

    bool WindowsInput::IsMouseButtonPressedImpl(int button)
    {
        int x, y;
        Uint32 mouseState = SDL_GetMouseState(&x, &y);
        
        return (mouseState & SDL_BUTTON(button)) != 0;
    }

    Point2 WindowsInput::GetMousePosImpl()
    {
        int x, y;
        SDL_GetGlobalMouseState(&x, &y);

        return Point2(x, y);
    }

    Point2 WindowsInput::GetRelativeMousePosImpl()
    {
        int x, y;
        SDL_GetMouseState(&x, &y);

        return Point2(x, y);
    }

    int WindowsInput::GetMouseXImpl()
    {
        return GetMousePosImpl().x;
    }

    int WindowsInput::GetMouseYImpl()
    {
        return GetMousePosImpl().y;
    }

    Point2 WindowsInput::GetMouseDeltaImpl()
    {
        Point2 mousePos;
        SDL_GetMouseState(&mousePos.x, &mousePos.y);

        auto delta = Point2(mousePos.x - LastMousePosition.x, mousePos.y - LastMousePosition.y);

        LastMousePosition = mousePos;
        
        return delta;
    }
}

