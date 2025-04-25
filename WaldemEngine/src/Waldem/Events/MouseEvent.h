#pragma once

#include "Event.h"

namespace Waldem
{
    class WALDEM_API MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y) : MouseX(x), MouseY(y) {}
        
        inline float GetX() const { return MouseX; }
        inline float GetY() const { return MouseY; }

        WString ToString() const override
        {
            std::stringstream ss;
            ss << "MouseMovedEvent: " << MouseX << ", " << MouseY;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    private:
        float MouseX, MouseY;
    };
    
    class WALDEM_API MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float xOffset, float yOffset) : XOffset(xOffset), YOffset(yOffset) {}
        
        inline float GetXOffset() const { return XOffset; }
        inline float GetYOffset() const { return YOffset; }

        WString ToString() const override
        {
            std::stringstream ss;
            ss << "MouseScrolledEvent: " << XOffset << ", " << YOffset;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    private:
        float XOffset, YOffset;
    };
    
    class WALDEM_API MouseButtonEvent : public Event
    {
    public:
        inline int GetMouseButton() const { return Button; }
        
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput)
    protected:
        MouseButtonEvent(int button) : Button(button) {}
        
        int Button;
    };
    
    class WALDEM_API MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

        WString ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << Button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };
    
    class WALDEM_API MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

        WString ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << Button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };
}
