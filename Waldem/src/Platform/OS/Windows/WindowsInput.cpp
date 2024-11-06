#include "wdpch.h"
#include "WindowsInput.h"

#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Waldem/Application.h"

namespace Waldem
{
    Input* Input::Instance = new WindowsInput();
    
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

    std::pair<float, float> WindowsInput::GetMousePosImpl()
    {
        int x, y;
        SDL_GetMouseState(&x, &y);

        return std::pair((float)x, (float)y);
    }

    float WindowsInput::GetMouseXImpl()
    {
        auto[x, y] = GetMousePosImpl();

        return x;
    }

    float WindowsInput::GetMouseYImpl()
    {
        auto[x, y] = GetMousePosImpl();

        return y;
    }
}

