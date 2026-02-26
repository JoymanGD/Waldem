#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    class MenuBarWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        Path CurrentScenePath;
        
    public:
        MenuBarWidget() {}

        void Initialize(InputManager* inputManager) override
        {
        }

        void ExportScene(Path& path)
        {
            SceneManager::GetCurrentScene()->Serialize(path);
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New scene"))
                    {
                        SceneManager::NewScene();
                    }
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
        }
    };
}
