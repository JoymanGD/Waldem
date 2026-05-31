#pragma once
#include <array>
#include <SDL_scancode.h>
#include "Waldem/Input/Input.h"

namespace Waldem
{
    class WindowsInput : public Input
    {
    protected:
        virtual void UpdateImpl() override;
        virtual bool GetKeyImpl(int keycode) override;
        virtual bool GetKeyDownImpl(int keycode) override;
        virtual bool GetMouseImpl(int button) override;
        virtual bool GetMouseDownImpl(int button) override;
        virtual Point2 GetMousePosImpl() override;
        virtual Point2 GetRelativeMousePosImpl() override;
        virtual int GetMouseXImpl() override;
        virtual int GetMouseYImpl() override;
        virtual Point2 GetMouseDelta();
        virtual Point2 GetMouseDeltaImpl() override { return MouseDelta; }

        std::array<uint8_t, SDL_NUM_SCANCODES> PreviousKeyStates = {};
        std::array<uint8_t, SDL_NUM_SCANCODES> CurrentKeyStates = {};
        uint32_t PreviousMouseButtons = 0;
        uint32_t CurrentMouseButtons = 0;
        uint16_t PreviousModState = 0;
        uint16_t CurrentModState = 0;
        Point2 LastMousePosition = { 0.0f, 0.0f };
        Point2 MouseDelta = { 0.0f, 0.0f };
    };
}
    
