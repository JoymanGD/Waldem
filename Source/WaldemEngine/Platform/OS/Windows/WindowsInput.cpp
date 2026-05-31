#include "wdpch.h"
#include "WindowsInput.h"

#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Waldem/Engine.h"

namespace Waldem
{
    namespace
    {
        bool GetModifierState(int keycode, uint16_t modState)
        {
            if(keycode == SDLK_LSHIFT)   return (modState & KMOD_LSHIFT) != 0;
            if(keycode == SDLK_RSHIFT)   return (modState & KMOD_RSHIFT) != 0;
            if(keycode == SDLK_LCTRL)    return (modState & KMOD_LCTRL)  != 0;
            if(keycode == SDLK_RCTRL)    return (modState & KMOD_RCTRL)  != 0;
            if(keycode == SDLK_LALT)     return (modState & KMOD_LALT)   != 0;
            if(keycode == SDLK_RALT)     return (modState & KMOD_RALT)   != 0;
            if(keycode == SDLK_LGUI)     return (modState & KMOD_LGUI)   != 0;
            if(keycode == SDLK_RGUI)     return (modState & KMOD_RGUI)   != 0;
            if(keycode == SDLK_CAPSLOCK) return (modState & KMOD_CAPS)   != 0;

            return false;
        }

        bool GetKeyStateFromSnapshot(int keycode, const std::array<uint8_t, SDL_NUM_SCANCODES>& keyStates, uint16_t modState)
        {
            if(GetModifierState(keycode, modState))
            {
                return true;
            }

            SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode);
            if(scancode < 0 || scancode >= SDL_NUM_SCANCODES)
            {
                return false;
            }

            return keyStates[scancode] != 0;
        }
    }

    void Input::Initialize()
    {
        Instance = new WindowsInput();
    }

    void Input::Update()
    {
        if(Instance != nullptr)
        {
            Instance->UpdateImpl();
        }
    }

    void WindowsInput::UpdateImpl()
    {
        PreviousKeyStates = CurrentKeyStates;
        PreviousMouseButtons = CurrentMouseButtons;
        PreviousModState = CurrentModState;

        int keyCount = 0;
        const uint8_t* state = SDL_GetKeyboardState(&keyCount);
        CurrentKeyStates.fill(0);
        for(int i = 0; i < keyCount && i < SDL_NUM_SCANCODES; ++i)
        {
            CurrentKeyStates[i] = state[i];
        }

        CurrentMouseButtons = SDL_GetMouseState(nullptr, nullptr);
        CurrentModState = static_cast<uint16_t>(SDL_GetModState());

        MouseDelta = GetMouseDelta();
    }

    bool WindowsInput::GetKeyImpl(int keycode)
    {
        return GetKeyStateFromSnapshot(keycode, CurrentKeyStates, CurrentModState);
    }

    bool WindowsInput::GetKeyDownImpl(int keycode)
    {
        const bool isDownNow = GetKeyStateFromSnapshot(keycode, CurrentKeyStates, CurrentModState);
        const bool wasDownBefore = GetKeyStateFromSnapshot(keycode, PreviousKeyStates, PreviousModState);
        return isDownNow && !wasDownBefore;
    }

    bool WindowsInput::GetMouseImpl(int button)
    {
        return (CurrentMouseButtons & SDL_BUTTON(button)) != 0;
    }

    bool WindowsInput::GetMouseDownImpl(int button)
    {
        const uint32_t buttonMask = SDL_BUTTON(button);
        return (CurrentMouseButtons & buttonMask) != 0 && (PreviousMouseButtons & buttonMask) == 0;
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

    Point2 WindowsInput::GetMouseDelta()
    {
        Point2 mousePos;
        SDL_GetMouseState(&mousePos.x, &mousePos.y);

        auto delta = Point2(mousePos.x - LastMousePosition.x, mousePos.y - LastMousePosition.y);

        LastMousePosition = mousePos;
        
        return delta;
    }
}

