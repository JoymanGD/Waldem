#pragma once

#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/ECS/Systems/System.h"
#include "Editor/Widgets/HierarchyWidget.h"
#include "Editor/Widgets/MainWidgetContainer.h"
#include "Editor/Widgets/EntityDetailsWidgetContainer.h"
#include "Editor/Widgets/ContentBrowserWidget.h"
#include "Editor/Widgets/MainViewportWidget.h"

namespace Waldem
{
    class WALDEM_API EditorUISystem : public ISystem
    {
        MainWidgetContainer* MainWidget = nullptr;
        Path CurrentScenePath;
        
    public:
        EditorUISystem()
        {
           MainWidget = new MainWidgetContainer(
           {
               new HierarchyWidget(),
               new MainViewportWidget(),
               new EntityDetailsWidgetContainer(),
               new ContentBrowserWidget(),
           });
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
            MainWidget->Initialize(inputManager, resourceManager);
        }

        void Deinitialize() override
        {
            MainWidget->Deinitialize();
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

        void ExportScene(Path& path)
        {
            SceneManager::GetCurrentScene()->Serialize(path);
        }

        void Update(float deltaTime) override
        {
            UpdateDockSpace();

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New scene")) {}
                    if (ImGui::MenuItem("Open scene", "Ctrl+O"))
                    {
                        if(OpenFile(CurrentScenePath))
                        {
                            SceneManager::LoadScene(CurrentScenePath);
                        }
                    }
                    if (ImGui::MenuItem("Save scene"))
                    {
                        bool save = true;
                        if(CurrentScenePath.empty())
                        {
                            save = SaveFile(CurrentScenePath);
                        }

                        if(save)
                        {
                            ExportScene(CurrentScenePath);
                        }
                    }
                    if (ImGui::MenuItem("Save scene as..."))
                    {
                        if(SaveFile(CurrentScenePath))
                        {
                            ExportScene(CurrentScenePath);
                        }
                    }
            
                    ImGui::Separator();
					       
                    if (ImGui::BeginMenu("Options"))
                    {
                        ImGui::EndMenu();
                    }
					       
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            MainWidget->OnDraw(deltaTime);
        }
    };
}
