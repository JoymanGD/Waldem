#pragma once

#include "Waldem/Core.h"
#include "Waldem/Events/Event.h"

namespace Waldem
{
    class WALDEM_API Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate() {}
        virtual void OnEvent(Event& event) {}

        inline const std::string& GetName() const { return DebugName; }
    protected:
        std::string DebugName;
    };
}
