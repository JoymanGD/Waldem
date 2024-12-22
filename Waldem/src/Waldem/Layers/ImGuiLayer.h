#pragma once

#include <SDL_mouse.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/ECS/Systems/EditorTransformsManipulationSystem.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer(Window* window, ecs::Manager* ecsManager, InputManager* inputManager) : Layer("ImGuiLayer", window, ecsManager, inputManager)
        {
			UISystems.Add((ISystem*)new EditorTransformsManipulationSystem(CoreECSManager));
        	
			SceneData sceneData = { window };
        	
            for (ISystem* system : UISystems)
            {
                system->Initialize(&sceneData, CurrentInputManager);
            }
        }
        
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
                event.Handled |= ImGuizmo::IsUsing();
            }
        }

        void OnDrawUI(float deltaTime) override
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New scene")) {}
                    if (ImGui::MenuItem("Open scene", "Ctrl+O")) {}
                    if (ImGui::MenuItem("Save scene")) {}

                    ImGui::Separator();
					
                    if (ImGui::BeginMenu("Options"))
                    {
                        ImGui::EndMenu();
                    }
					
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
            
            for (ISystem* system : UISystems)
            {
                system->Update(deltaTime);
            }
        }

    private:
		bool m_BlockEvents = true;
		WArray<ISystem*> UISystems;
    };
}