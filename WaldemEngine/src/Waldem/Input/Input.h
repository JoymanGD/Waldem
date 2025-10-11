#pragma once

#include "Waldem/Core.h"

namespace Waldem
{
    class WALDEM_API Input
    {
    public:
        static void Initialize();
        inline static bool IsKeyPressed(int keycode) { return Instance->IsKeyPressedImpl(keycode); }
        inline static bool IsMouseButtonPressed(int button) { return Instance->IsMouseButtonPressedImpl(button); }
        inline static Point2 GetMousePos() { return Instance->GetMousePosImpl(); }
        inline static Point2 GetRelativeMousePos() { return Instance->GetRelativeMousePosImpl(); }
        inline static int GetMouseX() { return Instance->GetMouseXImpl(); }
        inline static int GetMouseY() { return Instance->GetMouseYImpl(); }
        inline static Point2 GetMouseDelta() { return Instance->GetMouseDeltaImpl(); }
    protected:
        virtual bool IsKeyPressedImpl(int keycode) = 0;
        virtual bool IsMouseButtonPressedImpl(int button) = 0;
        virtual Point2 GetMousePosImpl() = 0;
        virtual Point2 GetRelativeMousePosImpl() = 0;
        virtual int GetMouseXImpl() = 0;
        virtual int GetMouseYImpl() = 0;
        virtual Point2 GetMouseDeltaImpl() = 0;
        inline static Input* Instance;
    };
}