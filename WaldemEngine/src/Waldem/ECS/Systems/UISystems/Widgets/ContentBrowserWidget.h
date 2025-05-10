#pragma once
#include "Waldem/ECS/Systems/UISystems/Widgets/IWidgetContainerSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API ContentBrowserWidget : public IWidgetSystem
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        
    public:
        ContentBrowserWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        WString GetName() override { return "Content"; }

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            if (ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                static char searchBuffer[128] = "";
                ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                ImGui::Separator();

                ImGui::BeginChild("AssetList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

                //change to caching instead of iterating every frame
                for (const auto& entry : std::filesystem::recursive_directory_iterator(CONTENT_PATH))
                {
                    if (entry.is_regular_file() && entry.path().extension() == ".ass")
                    {
                        if (strlen(searchBuffer) > 0 && entry.path().filename().string().find(searchBuffer) == std::string::npos)
                            continue;

                        if (ImGui::Selectable(entry.path().filename().string().c_str()))
                        {
                        }
                    }
                }

                ImGui::EndChild();
            }
            ImGui::End();
        }
    };
}
