#pragma once
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API ContentBrowserWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        char SearchBuffer[128] = "";
        Path CurrentPath = CONTENT_PATH;
        std::optional<Path> HoveredDropTargetFolder;
        std::optional<Path> SelectedFolderTreePath;
        std::optional<Path> SelectedAssetListPath;
        float CellSize = 100.0f;
        float Padding = 16.0f;
        
    public:
        ContentBrowserWidget() {}

        WString GetName() override { return "Content"; }

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            contentManager->SubscribeToFileDroppedEvent([contentManager, this](Path path)
            {
                Path target = HoveredDropTargetFolder.has_value() ? HoveredDropTargetFolder.value() : CurrentPath;
                
                if(!target.empty())
                {
                    contentManager->ImportTo(path, target);
                    SelectedAssetListPath = target;
                }
            });
        }

        void RenderFolderTree(const Path& path)
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_directory())
                {
                    const std::string folderName = entry.path().filename().string() + "/";

                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                    if (SelectedFolderTreePath.has_value() && SelectedFolderTreePath.value() == entry.path())
                        flags |= ImGuiTreeNodeFlags_Selected;

                    bool isOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);
                    if (ImGui::IsItemClicked())
                    {
                        SelectedFolderTreePath = entry.path();
                        CurrentPath = entry.path();
                    }

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                    {
                        HoveredDropTargetFolder = entry.path();
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

                    if (isOpen)
                    {
                        RenderFolderTree(entry.path()); // Recursive call for subfolders
                        ImGui::TreePop();
                    }
                }
            }
        }

        void RenderAssetsList()
        {            
            if (CurrentPath != CONTENT_PATH)
            {
                ImGui::Selectable("..");

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    CurrentPath = CurrentPath.parent_path();
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
                        std::filesystem::path newFolderPath = CurrentPath / folderName;
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

            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columnCount = (int)(panelWidth / (CellSize + Padding));
            if (columnCount < 1) columnCount = 1;

            int itemIndex = 0;

            for (const auto& entry : std::filesystem::directory_iterator(CurrentPath))
            {
                WString extension = entry.path().extension().string();

                bool isFolder = entry.is_directory();
                
                if(!IsSupportedExtension(extension) && !isFolder)
                {
                    continue;
                }
                
                const std::string itemName = entry.path().filename().string();
                if (strlen(SearchBuffer) > 0 && itemName.find(SearchBuffer) == std::string::npos)
                {
                    continue;
                }

                ImGui::BeginGroup();

                std::string id = "##Icon_" + entry.path().string();

                // Simulate icon box
                if (ImGui::Button(id.c_str(), ImVec2(CellSize, CellSize)))
                {
                    
                }

                // --- Selection on single click ---
                if (ImGui::IsItemClicked())
                {
                    SelectedAssetListPath = entry.path(); // Works for both files and folders
                }

                // --- Highlight selected item ---
                if (SelectedAssetListPath.has_value() && SelectedAssetListPath.value() == entry.path())
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    drawList->AddRect(
                        ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                        IM_COL32(255, 255, 0, 255), 4.0f // Yellow border
                    );
                }

                // --- Double-click to enter folder or open file ---
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (isFolder)
                        CurrentPath = entry.path(); // Enter folder
                    else
                    {
                        // Handle opening the asset (if applicable)
                    }
                }

                // If it's a folder and being hovered, remember it
                if (isFolder && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                {
                    HoveredDropTargetFolder = entry.path();
                }
                
                // Drag and drop
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    std::string fullPath = std::filesystem::relative(entry.path(), CONTENT_PATH).string();
                    
                    ImGui::SetDragDropPayload(ExtensionToAssetString(extension), fullPath.c_str(), fullPath.size() + 1);
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
                        CurrentPath = entry.path();
                    else
                    {
                        // Handle asset open
                    }
                }

                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + CellSize);
                ImGui::TextWrapped("%s", itemName.c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndGroup();

                // Layout in grid
                itemIndex++;
                if (itemIndex % columnCount != 0)
                    ImGui::SameLine();
            }

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGui::IsAnyItemHovered())
            {
                SelectedAssetListPath.reset(); // Deselect
            }
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                HoveredDropTargetFolder.reset();
                
                //search bar
                ImGui::InputTextWithHint("##Search", "Search...", SearchBuffer, IM_ARRAYSIZE(SearchBuffer));
                ImGui::Separator();

                //folders tree
                ImGui::BeginChild("FoldersTree", ImVec2(200, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                RenderFolderTree(CONTENT_PATH);
                ImGui::EndChild();

                ImGui::SameLine();

                //assets list
                ImGui::BeginChild("AssetList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                RenderAssetsList();
                ImGui::EndChild();
            }
            ImGui::End();
        }
    };
}
