#pragma once

#include "Waldem/Core.h"
#include "Waldem/Events/Event.h"

namespace Waldem
{
    class WALDEM_API Layer
    {
    public:
        Layer(const String& name = "Layer") { DebugName = name; }
        virtual ~Layer() = default;
        virtual void Begin() {}
        virtual void End() {}
        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnDrawUI(float deltaTime) {}
        virtual void OnEvent(Event& event) {}

        inline const String& GetName() const { return DebugName; }
    protected:
        String DebugName;
    };
}
