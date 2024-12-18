#pragma once
#include "Waldem/Events/Event.h"

namespace Waldem
{
    class WALDEM_API InputManager
    {
    public:
        void Broadcast(Event& event)
        {
            for (auto& handler : EventHandlers[event.GetEventType()])
            {
                handler();
            }
        }
    private:
        
        std::unordered_map<EventType, WArray<std::function<void()>>> EventHandlers;
    };
}
