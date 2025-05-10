#pragma once

#include "Waldem/Core.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        AppTick,
        AppUpdate,
        AppRender,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
        FileDropped,
    };

    enum EventCategory
    {
        None = 0,
        EventCategoryApplication = BIT(0),
        EventCategoryInput = BIT(1),
        EventCategoryKeyboard = BIT(2),
        EventCategoryMouse = BIT(3),
        EventCategoryMouseButton = BIT(4),
        EventCategoryFile = BIT(5)
    };

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
                                virtual EventType GetEventType() const override { return GetStaticType(); }\
                                virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }
    
    class WALDEM_API Event
    {
        friend class EventDispatcher;
    public:
        virtual ~Event() = default;
        
        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual WString ToString() const { return GetName(); }

        inline bool IsInCategory(EventCategory category)
        {
            return GetCategoryFlags() & category;
        }
        
        bool Handled = false;
    };

    class EventDispatcher
    {
        template<typename T>
        using EventFn = std::function<bool(T&)>;
    public:
        EventDispatcher(Event& event) : Event(event)
        {
            
        }

        template<typename T>
        bool Dispatch(EventFn<T> func)
        {
            if(Event.GetEventType() == T::GetStaticType())
            {
                Event.Handled = func(*(T*)&Event);
                return true;
            }
            return false;
        }
    private:
        Event& Event;
    };

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
    return os << e.ToString();
}
}

