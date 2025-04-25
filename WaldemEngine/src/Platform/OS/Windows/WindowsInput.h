#pragma once
#include "Waldem/Input/Input.h"

namespace Waldem
{
    class WindowsInput : public Input
    {
    protected:
        virtual bool IsKeyPressedImpl(int keycode) override;
        virtual bool IsMouseButtonPressedImpl(int button) override;
        virtual std::pair<float, float> GetMousePosImpl() override;
        virtual float GetMouseXImpl() override;
        virtual float GetMouseYImpl() override;
        virtual Point2 GetMouseDeltaImpl() override;

        Point2 LastMousePosition = { 0.0f, 0.0f };
    };
}
    
