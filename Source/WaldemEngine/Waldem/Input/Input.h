#pragma once

#include "Waldem/Core.h"

namespace Waldem
{
    class WALDEM_API Input
    {
    public:
        static void Initialize();
        static void Update();
        inline static bool GetKey(int keycode) { return Instance->GetKeyImpl(keycode); }
        inline static bool GetKeyDown(int keycode) { return Instance->GetKeyDownImpl(keycode); }
        inline static bool GetMouse(int button) { return Instance->GetMouseImpl(button); }
        inline static bool GetMouseDown(int button) { return Instance->GetMouseDownImpl(button); }
        inline static Point2 GetMousePos() { return Instance->GetMousePosImpl(); }
        inline static Point2 GetRelativeMousePos() { return Instance->GetRelativeMousePosImpl(); }
        inline static int GetMouseX() { return Instance->GetMouseXImpl(); }
        inline static int GetMouseY() { return Instance->GetMouseYImpl(); }
        inline static Point2 GetMouseDelta() { return Instance->GetMouseDeltaImpl(); }
    protected:
        virtual void UpdateImpl() = 0;
        virtual bool GetKeyImpl(int keycode) = 0;
        virtual bool GetKeyDownImpl(int keycode) = 0;
        virtual bool GetMouseImpl(int button) = 0;
        virtual bool GetMouseDownImpl(int button) = 0;
        virtual Point2 GetMousePosImpl() = 0;
        virtual Point2 GetRelativeMousePosImpl() = 0;
        virtual int GetMouseXImpl() = 0;
        virtual int GetMouseYImpl() = 0;
        virtual Point2 GetMouseDeltaImpl() = 0;
        inline static Input* Instance;
    };
}
