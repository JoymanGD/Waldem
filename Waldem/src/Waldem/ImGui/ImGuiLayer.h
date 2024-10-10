#pragma once
#include "Waldem/Layer.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Events/MouseEvent.h"

namespace Waldem
{
    class WALDEM_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();
        
        void Begin() override;
        void End() override;
        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& event) override;
        void OnUIRender() override;
    private:
		bool m_BlockEvents = false;
    };
}