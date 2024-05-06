#pragma once
#include "Waldem/Layer.h"

namespace Waldem
{
    class WALDEM_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate() override;
        void OnEvent(Event& event) override;
    private:
        
    };
}
