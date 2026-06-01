#pragma once
#include "imgui.h"
#include "imgui_stdlib.h"
#include "Widget.h"
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#include "Waldem/ProjectManagement/ProjectManager.h"

namespace Waldem
{
    class CreateProjectWidget : public IWidget
    {
    private:
        inline static bool Visible = false;
        char NameBuffer[64] = "NewProject";
        char PathBuffer[512] = "C:/WaldemProjects";

    public:
        CreateProjectWidget() = default;

        static bool IsVisible()
        {
            return Visible;
        }

        static void SetVisible(bool visible)
        {
            Visible = visible;
        }

        void Initialize(InputManager* inputManager) override
        {
        }

        void OnDraw(float deltaTime) override
        {
            if(!Visible)
            {
                return;
            }

            const bool isVisible = ImGui::Begin("CreateProject###CreateProject", &Visible);
            if (!isVisible)
            {
                ImGui::End();
                return;
            }

            ImGui::InputText("Name", NameBuffer, IM_ARRAYSIZE(NameBuffer));
            ImGui::InputText("Path", PathBuffer, IM_ARRAYSIZE(PathBuffer));

            ImGui::SameLine();
            if(ImGui::Button("..."))
            {
                Path selected(PathBuffer);
                
                if(SelectFolder(selected))
                {
                    strcpy_s(PathBuffer, sizeof(PathBuffer), selected.string().c_str());
                }
            }
            
            if (ImGui::Button("Create"))
            {
                ProjectManager::CreateProject(WString(NameBuffer), Path(PathBuffer));
                if(ProjectManager::HasProject())
                {
                    Path startupScenePath = ProjectManager::CurrentProject.GetStartupScenePath();
                    SceneManager::LoadScene(startupScenePath);
                }
                
                Visible = false;
            }

            ImGui::End();
        }
    };
}
