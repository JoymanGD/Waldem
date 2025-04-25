#pragma once

#include "Event.h"

namespace Waldem
{
    class WALDEM_API WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(float width, float height) : Width(width), Height(height) {}
        
        inline float GetWidth() const { return Width; }
        inline float GetHeight() const { return Height; }
        inline std::array<float, 2> GetSize() const { return { Width, Height }; }

        WString ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << Width << ", " << Height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        float Width, Height;
    };
    
    class WALDEM_API WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() {}
        
        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
    
    class WALDEM_API AppTickEvent : public Event
    {
    public:
        AppTickEvent() {}
        
        EVENT_CLASS_TYPE(AppTick)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
    
    class WALDEM_API AppUpdateEvent : public Event
    {
    public:
        AppUpdateEvent() {}
        
        EVENT_CLASS_TYPE(AppUpdate)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
    
    class WALDEM_API AppRenderEvent : public Event
    {
    public:
        AppRenderEvent() {}
        
        EVENT_CLASS_TYPE(AppRender)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}
