#pragma once
#include "Shortcut.h"
#include "../../../WaldemEditor/EditorShortcuts.h"
#include "Waldem/Events/Event.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Events/MouseEvent.h"

namespace Waldem
{
    using KeyEventHandler = std::function<void(bool)>;
    using MouseButtonEventHandler = std::function<void(bool)>;
    using MouseMoveEventHandler = std::function<void(Vector2)>;
    using MouseScrollEventHandler = std::function<void(Vector2)>;
    using ShortcutHandler = std::function<void()>;
    using DynamicShortcutProvider = std::function<Shortcut()>;
    using ShortcutPredicate = std::function<bool()>;
    
    struct DynamicShortcutEntry
    {
        DynamicShortcutProvider Provider;
        ShortcutHandler Handler;
        ShortcutPredicate Predicate;
    };
    
    struct EditorShortcutEntry
    {
        EditorShortcutAction Action;
        ShortcutHandler Handler;
        ShortcutPredicate Predicate;
    };
    
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
        
        void SubscribeToShortcut(const Shortcut& shortcut, ShortcutHandler handler)
        {
            ShortcutHandlers[shortcut].Add(handler);
        }

        void SubscribeToDynamicShortcut(DynamicShortcutProvider provider, ShortcutHandler handler, ShortcutPredicate predicate = {})
        {
            DynamicShortcutHandlers.Add({provider, handler, predicate});
        }

        void SubscribeToEditorShortcut(EditorShortcutAction action, ShortcutHandler handler, ShortcutPredicate predicate = {})
        {
            EditorShortcutHandlers.Add({action, handler, predicate});
        }
        
        bool Broadcast(Event& event, bool blockShortcuts = false)
        {
            bool handled = false;
            
            auto eventType = event.GetEventType();
            
            switch (eventType)
            {
            case EventType::KeyPressed:
                {
                    KeyEvent& keyEvent = static_cast<KeyEvent&>(event);
                    PressedKeys.insert(keyEvent.GetKeyCode());

                    if(!blockShortcuts)
                    {
                        for (auto& [shortcut, handlers] : ShortcutHandlers)
                        {
                            if (shortcut.Matches(PressedKeys))
                            {
                                for (auto& handler : handlers)
                                    handler();
                            }
                        }

                        for (auto& entry : EditorShortcutHandlers)
                        {
                            Shortcut shortcut = EditorShortcuts::GetShortcut(entry.Action);
                            if (shortcut.Matches(PressedKeys) && (!entry.Predicate || entry.Predicate()))
                            {
                                entry.Handler();
                            }
                        }

                        for (auto& entry : DynamicShortcutHandlers)
                        {
                            Shortcut shortcut = entry.Provider();
                            if (shortcut.Matches(PressedKeys) && (!entry.Predicate || entry.Predicate()))
                            {
                                entry.Handler();
                            }
                        }
                    }

                    for (auto& handler : KeyEventHandlers[keyEvent.GetKeyCode()])
                        handler(true);

                    handled = !KeyEventHandlers[keyEvent.GetKeyCode()].IsEmpty();
                    break;
                }
            case EventType::KeyReleased:
                {
                    KeyEvent& keyEvent = static_cast<KeyEvent&>(event);
                    PressedKeys.erase(keyEvent.GetKeyCode());

                    for (auto& handler : KeyEventHandlers[keyEvent.GetKeyCode()])
                        handler(false);

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
        std::unordered_map<Shortcut, WArray<ShortcutHandler>, ShortcutHash> ShortcutHandlers;
        
        WArray<DynamicShortcutEntry> DynamicShortcutHandlers;
        WArray<EditorShortcutEntry> EditorShortcutHandlers;
        WArray<MouseMoveEventHandler> MouseMoveEventHandlers;
        WArray<MouseScrollEventHandler> MouseScrollEventHandlers;
        
        std::unordered_set<int> PressedKeys;
    };
}
