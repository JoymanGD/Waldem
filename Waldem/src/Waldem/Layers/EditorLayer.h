#pragma once

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/ECS/Systems/DebugSystem.h"
#include "Waldem/ECS/Systems/EditorSystems/GuizmoEditorSystem.h"
#include "Waldem/ECS/Systems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Systems/EditorSystems/EntityDetailsWidgetContainer.h"
#include "Waldem/ECS/Systems/EditorSystems/MainWidgetContainer.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/HierarchyWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/OceanComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/TransformComponentWidget.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API EditorLayer : public Layer
    {
    private:
        MainWidgetContainer* MainWidgetContainerSystem;
    public:
        EditorLayer(Window* window, ecs::Manager* ecsManager, ResourceManager* resourceManager) : Layer("EditorLayer", window, ecsManager, resourceManager)
        {
            EditorInputManager = {};
            
            auto cameraEntity = CurrentECSManager->CreateEntity("Camera");
            float aspectRatio = window->GetWidth() / window->GetHeight();
            cameraEntity.Add<Transform>(Vector3(0, 0, 0));
            cameraEntity.Add<Camera>(70.0f, aspectRatio, 0.1f, 100.0f, 30.0f, 30.0f);
            cameraEntity.Add<MainCamera>();
            
            //do it after all entities set up
            CurrentECSManager->Refresh();

            MainWidgetContainerSystem = new MainWidgetContainer(CurrentECSManager,
            {
                new EntityDetailsWidgetContainer(CurrentECSManager,
                {
                    new OceanComponentWidget(CurrentECSManager),
                    new TransformComponentWidget(CurrentECSManager)
                }),
                new HierarchyWidget(CurrentECSManager)
            });
            
            UISystems.Add(new GuizmoEditorSystem(CurrentECSManager));
            UpdateSystems.Add((ISystem*)new FreeLookCameraSystem(CurrentECSManager));
        	
			SceneData sceneData = { window };

            MainWidgetContainerSystem->Initialize(&sceneData, &EditorInputManager, CurrentResourceManager);
        	
            for (ISystem* system : UISystems)
            {
                system->Initialize(&sceneData, &EditorInputManager, CurrentResourceManager);
            }
        	
            for (ISystem* system : UpdateSystems)
            {
                system->Initialize(&sceneData, &EditorInputManager, CurrentResourceManager);
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
            InitializeUI();
        }
        
        void OnDetach() override
        {
            DeinitializeUI();
        }
        
        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            if (BlockUIEvents)
            {
                ImGuiIO& io = ImGui::GetIO();
                event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
                event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
                event.Handled |= ImGuizmo::IsUsing();
            }
            
            switch (eventType)
            {
            case EventType::MouseMoved:
                {
                    EditorInputManager.Broadcast(event);
                    break;
                }
            case EventType::MouseScrolled:
            case EventType::KeyPressed:
            case EventType::KeyReleased:
            case EventType::KeyTyped:
                {
                    event.Handled = EditorInputManager.Broadcast(event);
                    break;
                }
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
                {
                    if(event.Handled) //for these events we dont want to broadcast them if they are already handled by UI handlers to avoid counter-behaving
                    {
                        break;
                    }
                    
                    event.Handled = EditorInputManager.Broadcast(event);
                    break;
                }
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
            
            MainWidgetContainerSystem->Update(deltaTime);
            
            for (ISystem* system : UISystems)
            {
                system->Update(deltaTime);
            }
        }

        void OnUpdate(float deltaTime) override
        {
            for (ISystem* system : UpdateSystems)
            {
                system->Update(deltaTime);
            }
        }

        void InitializeUI()
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

        void DeinitializeUI()
        {
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
        }

    private:
		bool BlockUIEvents = true;
		WArray<ISystem*> UISystems;
		WArray<ISystem*> UpdateSystems;
        InputManager EditorInputManager;
    };
}