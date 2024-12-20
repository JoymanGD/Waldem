#pragma once
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Events/MouseEvent.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer(Window* window) : Layer("ImGuiLayer", window) {}
        
        void Begin() override
        {
            Renderer::BeginUI();
        }
        
        void End() override
        {
            Renderer::EndUI();
        }
        
        void OnAttach() override
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
            // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking
            // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable multi-viewport

            SDL_Window* window = static_cast<SDL_Window*>(Window::GetNativeWindow());
            ImGui_ImplSDL2_InitForD3D(window);

            // Style
            ImGui::StyleColorsDark();

            Renderer::InitializeUI();
        }
        
        void OnDetach() override
        {
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
        }
        
        void OnEvent(Event& event) override
        {
            if (m_BlockEvents)
            {
                ImGuiIO& io = ImGui::GetIO();
                event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
                event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
            }
        }
        
    private:
		bool m_BlockEvents = true;
    };
}
