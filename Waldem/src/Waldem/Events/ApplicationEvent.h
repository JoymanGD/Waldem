#pragma once

#include "Event.h"

namespace Waldem
{
    class WALDEM_API WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height) : Width(width), Height(height) {}
        
        inline uint32_t GetWidth() const { return Width; }
        inline uint32_t GetHeight() const { return Height; }
        inline std::tuple<uint32_t, uint32_t> GetSize() const { return std::tuple(Width, Height); }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << Width << ", " << Height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        uint32_t Width, Height;
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
