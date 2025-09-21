#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "imgui.h"
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Systems/EditorSystems/EditorControlSystem.h"
#include "Waldem/ECS/Systems/EditorSystems/EditorGuizmoSystem.h"
#include "Waldem/ECS/Systems/GameSystems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Systems/CoreSystems/OceanSimulationSystem.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Utils/FileUtils.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Systems/CoreSystems/AnimationSystem.h"
#include "Waldem/ECS/Systems/CoreSystems/HybridRenderingSystem.h"
#include "Waldem/Editor/Widgets/ContentBrowserWidget.h"
#include "Waldem/Editor/Widgets/ViewportWidget.h"
#include "Waldem/Editor/Widgets/EntityDetailsWidgetContainer.h"
#include "Waldem/Editor/Widgets/HierarchyWidget.h"
#include "Waldem/Editor/Widgets/MenuBarWidget.h"

namespace Waldem
{
    class WALDEM_API EditorLayer : public Layer
    {
    private:
        CWindow* Window;
        bool BlockUIEvents = true;
        bool ImportSceneThisFrame = false;
        bool EditorViewportResizeTriggered = false;
        Vector2 NewEditorViewportSize = {};
        WArray<IWidget*> Widgets;
        
    public:
        EditorLayer(CWindow* window) : Layer("EditorLayer", window)
        {
            InputManager = {};
            
            Vector2 size = { 1920, 1080 };
            auto cameraEntity = ECS::CreateEntity("EditorCamera");
            float aspectRatio = size.x / size.y;
            cameraEntity.set<Transform>({Vector3(0, 10, -10.f)});
            cameraEntity.set<Camera>({60.0f, aspectRatio, 0.001f, 1000.0f, 30.0f, 30.0f});
            cameraEntity.add<AudioListener>();
            cameraEntity.add<EditorComponent>();
            ViewportManager::GetEditorViewport()->LinkCamera(cameraEntity);

            auto skyEntity = ECS::CreateEntity("Sky");
            skyEntity.add<Transform>();
            skyEntity.add<Sky>();

            Widgets.Add(new MenuBarWidget());
            Widgets.Add(new ViewportWidget(ViewportManager::GetEditorViewport()));
            Widgets.Add(new ViewportWidget(ViewportManager::GetGameViewport()));
            Widgets.Add(new HierarchyWidget());
            Widgets.Add(new EntityDetailsWidgetContainer());
            Widgets.Add(new ContentBrowserWidget());
            
            //do it after all entities set up
            UpdateSystems.Add(new EditorGuizmoSystem());
            UpdateSystems.Add(new EditorControlSystem());

            Window = window;

            ViewportManager::GetEditorViewport()->SubscribeOnResize([this](Vector2 size)
            {
                OnResize(size);
            });

            ECS::World.system("WidgetDrawing").kind(flecs::OnGUI).each([&]
            {
                UpdateDockSpace();
                
                for (auto widget : Widgets)
                {
                    widget->OnDraw(Time::DeltaTime);
                }
            });
        }

        void OnResize(Vector2 size)
        {
            ECS::Entity linkedCamera;
            if(ViewportManager::GetEditorViewport()->TryGetLinkedCamera(linkedCamera))
            {
                auto camera = linkedCamera.get_mut<Camera>();
                camera->UpdateProjectionMatrix(camera->FieldOfView, size.x/size.y, camera->NearPlane, camera->FarPlane);
                linkedCamera.modified<Camera>(); 
            }
        }
        
        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            if (BlockUIEvents)
            {
                event.Handled |= event.IsInCategory(EventCategoryKeyboard) & ImGui::GetIO().WantCaptureKeyboard;
                event.Handled |= ImGuizmo::IsUsing();
            }
            
            switch (eventType)
            {
            case EventType::MouseMoved:
                {
                    InputManager.Broadcast(event);
                    break;
                }
            case EventType::MouseScrolled:
            case EventType::KeyPressed:
            case EventType::KeyReleased:
            case EventType::KeyTyped:
                {
                    event.Handled = InputManager.Broadcast(event);
                    break;
                }
            case EventType::MouseButtonPressed:
                {
                    event.Handled |= event.IsInCategory(EventCategoryMouse) & (ImGui::GetIO().WantCaptureMouse && !ViewportManager::GetEditorViewport()->IsMouseOver);
                }
            case EventType::MouseButtonReleased:
                {
                    if(event.Handled)
                    {
                        break;
                    }
                    
                    event.Handled = InputManager.Broadcast(event);
                    break;
                }
            case EventType::FileDropped:
                {
                    event.Handled = CContentManager::Broadcast(event);
                    break;
                }
            }
        }
        
        void Initialize() override
        {
            for (ISystem* system : UISystems)
            {
                system->Initialize(&InputManager);
            }
        	
            for (ISystem* system : UpdateSystems)
            {
                system->Initialize(&InputManager);
            }
        	
            for (ISystem* system : DrawSystems)
            {
                system->Initialize(&InputManager);
            }
			
            for (ISystem* system : PhysicsSystems)
            {
                system->Initialize(&InputManager);
            }
            
            for (auto widget : Widgets)
            {
                widget->Initialize(&InputManager);
            }

            Initialized = true;
        }
        
        void Deinitialize() override
        {
            for (ISystem* system : UISystems)
            {
                system->Deinitialize();
            }
        	
            for (ISystem* system : UpdateSystems)
            {
                system->Deinitialize();
            }
        	
            for (ISystem* system : DrawSystems)
            {
                system->Deinitialize();
            }
			
            for (ISystem* system : PhysicsSystems)
            {
                system->Deinitialize();
            }
            
            for (auto widget : Widgets)
            {
                widget->Deinitialize();
            }

            Initialized = false;
        }

        void UpdateDockSpace()
        {
            // 🪟 Fullscreen invisible host window for the dockspace
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);

            ImGuiWindowFlags host_window_flags =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
            ImGui::Begin("MainDockspaceHost", nullptr, host_window_flags); // Host window
            ImGui::PopStyleVar(3);

            ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

            ImGui::End();
            
            static bool dockspaceInitialized = false;
            
            // Build the layout once
            if(!dockspaceInitialized)
            {
                ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

                // Start from root dockspace
                ImGuiID dock_main_id = dockspace_id;

                // Split horizontally: left (Entities) and right side (everything else)
                ImGuiID dock_left, dock_right;
                ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, &dock_left, &dock_main_id); // 20% left

                // Split right side horizontally: right (Details) and center+bottom
                ImGuiID dock_details, dock_centerblock;
                ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, &dock_details, &dock_centerblock); // 25% right

                // Split centerblock vertically: bottom (Content) and center
                ImGuiID dock_bottom, dock_center;
                ImGui::DockBuilderSplitNode(dock_centerblock, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_center); // 25% bottom

                // Now assign windows to docks
                ImGui::DockBuilderDockWindow("Entities", dock_left);
                ImGui::DockBuilderDockWindow("Details", dock_details);
                ImGui::DockBuilderDockWindow("Content", dock_bottom);
                ImGui::DockBuilderDockWindow("Viewport", dock_center);
                // (Optional) nothing docked to center, can leave it empty or use a viewport/game window

                ImGui::DockBuilderFinish(dockspace_id);

                dockspaceInitialized = true;
            }
        }
    };
}
