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
                static std::filesystem::path contentPath = CONTENT_PATH;
                static std::filesystem::path currentPath = contentPath;
                
                static char searchBuffer[128] = "";

                ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                ImGui::Separator();

                ImGui::BeginChild("FoldersTree", ImVec2(200, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

                for (const auto& entry : std::filesystem::directory_iterator(contentPath))
                {
                    if (entry.is_directory())
                    {
                        if (strlen(searchBuffer) > 0 && entry.path().filename().string().find(searchBuffer) == std::string::npos)
                            continue;

                        const std::string folderName = entry.path().filename().string() + "/";

                        if (ImGui::Selectable(folderName.c_str()))
                        {
                            currentPath = entry.path();
                        }

                        // Context menu for folders
                        if (ImGui::BeginPopupContextItem())
                        {
                            if (ImGui::MenuItem("Delete"))
                            {
                                std::filesystem::remove_all(entry.path());
                            }
                            ImGui::EndPopup();
                        }
                    }
                }

                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("AssetList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

                if (currentPath != contentPath)
                {
                    ImGui::Selectable("..");

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        currentPath = currentPath.parent_path();
                    }
                }

                static bool openCreateNewFolderPopup = false;
                
                if (ImGui::BeginPopupContextWindow("CreateNewFolderContextWindow"))
                {
                    if (ImGui::MenuItem("Create Folder"))
                    {
                        openCreateNewFolderPopup = true;
                    }
                    
                    ImGui::EndPopup();
                }

                if(openCreateNewFolderPopup)
                {
                    ImGui::OpenPopup("CreateNewFolderPopup");
                    openCreateNewFolderPopup = false;
                }

                if (ImGui::BeginPopup("CreateNewFolderPopup"))
                {
                    static char folderName[128] = "";

                    ImGui::InputText("Folder Name", folderName, IM_ARRAYSIZE(folderName));

                    if (ImGui::Button("Create"))
                    {
                        if (strlen(folderName) > 0)
                        {
                            std::filesystem::path newFolderPath = currentPath / folderName;
                            if (!std::filesystem::exists(newFolderPath))
                                std::filesystem::create_directory(newFolderPath);
                        }

                        folderName[0] = '\0';
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        folderName[0] = '\0';
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                const float cellSize = 100.0f;
                const float padding = 16.0f;

                float panelWidth = ImGui::GetContentRegionAvail().x;
                int columnCount = (int)(panelWidth / (cellSize + padding));
                if (columnCount < 1) columnCount = 1;

                int itemIndex = 0;

                for (const auto& entry : std::filesystem::directory_iterator(currentPath))
                {
                    bool isFolder = entry.is_directory();
                    const std::string itemName = entry.path().filename().string();
                    if (strlen(searchBuffer) > 0 && itemName.find(searchBuffer) == std::string::npos)
                        continue;

                    ImGui::BeginGroup();

                    std::string id = "##Icon_" + entry.path().string();
                    // Simulate icon box
                    ImGui::Button(id.c_str(), ImVec2(cellSize, cellSize));
                    
                    // Drag and drop
                    if (entry.path().extension() == ".ass" && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        std::string fullPath = entry.path().string();
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET", fullPath.c_str(), fullPath.size() + 1);
                        ImGui::Text("%s", itemName.c_str());
                        ImGui::EndDragDropSource();
                    }

                    // Context menu
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Delete"))
                            std::filesystem::remove_all(entry.path());
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if (isFolder)
                            currentPath = entry.path();
                        else
                        {
                            // Handle asset open
                        }
                    }

                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellSize);
                    ImGui::TextWrapped("%s", itemName.c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::EndGroup();

                    // Layout in grid
                    itemIndex++;
                    if (itemIndex % columnCount != 0)
                        ImGui::SameLine();
                }

                ImGui::EndChild();
            }
            ImGui::End();
        }
    };
}
