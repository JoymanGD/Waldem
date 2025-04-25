#pragma once
#include "Waldem/Events/Event.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Events/MouseEvent.h"

namespace Waldem
{
    using KeyEventHandler = std::function<void(bool)>;
    using MouseButtonEventHandler = std::function<void(bool)>;
    using MouseMoveEventHandler = std::function<void(Vector2)>;
    using MouseScrollEventHandler = std::function<void(Vector2)>;
    
    class WALDEM_API InputManager
    {
    public:
        void SubscribeToKeyEvent(int keyCode, KeyEventHandler handler)
        {
            KeyEventHandlers[keyCode].Add(handler);
        }
        
        void SubscribeToMouseButtonEvent(int mouseButton, MouseButtonEventHandler handler)
        {
            MouseButtonEventHandlers[mouseButton].Add(handler);
        }
        
        void SubscribeToMouseMoveEvent(MouseMoveEventHandler handler)
        {
            MouseMoveEventHandlers.Add(handler);
        }
        
        void SubscribeToMouseScrollEvent(MouseScrollEventHandler handler)
        {
            MouseScrollEventHandlers.Add(handler);
        }
        
        bool Broadcast(Event& event)
        {
            bool handled = false;
            
            auto eventType = event.GetEventType();
            
            switch (eventType)
            {
            case EventType::KeyPressed:
            case EventType::KeyReleased:
                {
                    KeyEvent& keyEvent = static_cast<KeyEvent&>(event);
                    for (auto& handler : KeyEventHandlers[keyEvent.GetKeyCode()])
                    {
                        handler(eventType == EventType::KeyPressed);
                    }
                                        
                    handled = !KeyEventHandlers[keyEvent.GetKeyCode()].IsEmpty();
                    
                    break;
                }
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
                {
                    MouseButtonEvent& mouseButtonEvent = static_cast<MouseButtonEvent&>(event);
                    for (auto& handler : MouseButtonEventHandlers[mouseButtonEvent.GetMouseButton()])
                    {
                        handler(eventType == EventType::MouseButtonPressed);
                    }
                                        
                    handled = !MouseButtonEventHandlers[mouseButtonEvent.GetMouseButton()].IsEmpty();
                    
                    break;
                }
            case EventType::MouseMoved:
                {
                    MouseMovedEvent& mouseMovedEvent = static_cast<MouseMovedEvent&>(event);
                    
                    for (auto& handler : MouseMoveEventHandlers)
                    {
                        handler(Vector2(mouseMovedEvent.GetX(), mouseMovedEvent.GetY()));
                    }

                    handled = !MouseMoveEventHandlers.IsEmpty();
                    
                    break;
                }
            case EventType::MouseScrolled:
                {
                    MouseScrolledEvent& mouseScrolledEvent = static_cast<MouseScrolledEvent&>(event);

                    for (auto& handler : MouseScrollEventHandlers)
                    {
                        handler(Vector2(mouseScrolledEvent.GetXOffset(), mouseScrolledEvent.GetYOffset()));
                    }
                    
                    handled = !MouseMoveEventHandlers.IsEmpty();
                    
                    break;
                }
            }

            return handled;
        }
        
    private:
        std::unordered_map<int, WArray<KeyEventHandler>> KeyEventHandlers;
        std::unordered_map<int, WArray<MouseButtonEventHandler>> MouseButtonEventHandlers;
        WArray<MouseMoveEventHandler> MouseMoveEventHandlers;
        WArray<MouseScrollEventHandler> MouseScrollEventHandlers;
    };
}
