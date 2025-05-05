#pragma once
#include "imgui_internal.h"
#include "ComponentWidgets/ComponentWidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/IWidgetContainerSystem.h"
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

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            if (ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                // Search bar
                static char searchBuffer[128] = "";
                ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                ImGui::Separator();

                // Scrollable area for assets
                ImGui::BeginChild("AssetList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

                // Example assets (replace with actual asset data)
                const char* assets[] = { "Texture1.png", "Texture2.png", "Model1.obj", "Script1.cs", "Material1.mat" };

                for (int i = 0; i < IM_ARRAYSIZE(assets); i++)
                {
                    // Filter assets based on search
                    if (strlen(searchBuffer) > 0 && strstr(assets[i], searchBuffer) == nullptr)
                        continue;

                    // Display asset as selectable item
                    if (ImGui::Selectable(assets[i]))
                    {
                        // Handle asset selection logic here
                    }
                }

                ImGui::EndChild();
            }
            ImGui::End();
        }
    };
}
