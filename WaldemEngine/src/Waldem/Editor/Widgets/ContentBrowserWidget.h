#pragma once
#include "Widget.h"
#include "Waldem/SceneManagement/SceneManager.h"

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

        void Initialize(InputManager* inputManager) override
        {
            CContentManager::SubscribeToFileDroppedEvent([this](Path path)
            {
                Path target = HoveredDropTargetFolder.has_value() ? HoveredDropTargetFolder.value() : CurrentPath;
                
                if(!target.empty())
                {
                    CContentManager::ImportTo(path, target);
                    SelectedAssetListPath = target;
                }
            });
        }

        void RenderFolderTree(const Path& path)
        {
            ImGui::BeginChild("FolderTreeRegion", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_directory())
                {
                    const std::string folderName = entry.path().filename().string() + "/";

                    // Base flags
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                               ImGuiTreeNodeFlags_OpenOnDoubleClick | 
                                               ImGuiTreeNodeFlags_SpanAvailWidth;

                    if (SelectedFolderTreePath.has_value() && SelectedFolderTreePath.value() == entry.path())
                        flags |= ImGuiTreeNodeFlags_Selected;

                    // Check if this folder has subdirectories
                    bool hasChildren = false;
                    for (const auto& sub : std::filesystem::directory_iterator(entry.path()))
                    {
                        if (sub.is_directory()) { hasChildren = true; break; }
                    }
                    if (!hasChildren)
                        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

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

                    if (isOpen && hasChildren)
                    {
                        RenderFolderTree(entry.path()); // Recursive call for subfolders
                        ImGui::TreePop();
                    }
                }
            }

            // Reset selection if clicking on empty space
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && 
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
                !ImGui::IsAnyItemHovered())
            {
                SelectedFolderTreePath.reset();
                CurrentPath = CONTENT_PATH;
            }

            ImGui::EndChild();
        }


        std::string ToLower(const std::string& str)
        {
            std::string result = str;
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::tolower(c); });
            return result;
        }

        void RenderEntry(const std::filesystem::directory_entry& entry)
        {
            const std::string relativePath = relative(entry.path(), CONTENT_PATH).string();
            if (strlen(SearchBuffer) > 0)
            {
                std::string searchLower = ToLower(SearchBuffer);
                std::string pathLower   = ToLower(relativePath);

                if (pathLower.find(searchLower) == std::string::npos)
                {
                    return;
                }
            }

            WString extension = entry.path().extension().string();

            bool isFolder = entry.is_directory();
            
            if(!IsSupportedExtension(extension) && !isFolder)
            {
                return;
            }

            ImGui::BeginGroup();

            std::string id = "##Icon_" + entry.path().string();
            ImVec2 size(CellSize, CellSize);
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImVec2 rectMin = cursor;
            ImVec2 rectMax = ImVec2(cursor.x + size.x, cursor.y + size.y);

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Background
            drawList->AddRectFilled(rectMin, rectMax, IM_COL32(50, 50, 50, 255), 6.0f);

            // Click/hover handling
            bool clicked = ImGui::InvisibleButton(id.c_str(), size);

            if (entry.is_directory())
            {
                // Folder rectangle (yellow)
                ImVec2 folderMin(rectMin.x + 8, rectMin.y + 12);
                ImVec2 folderMax(rectMax.x - 8, rectMax.y - 8);

                drawList->AddRectFilled(folderMin, folderMax, IM_COL32(220, 180, 0, 255), 4.0f);
                drawList->AddRect(folderMin, folderMax, IM_COL32(255, 210, 50, 255), 4.0f);
            }
            else
            {
                // File rectangle (blue)
                ImVec2 fileMin(rectMin.x + 12, rectMin.y + 8);
                ImVec2 fileMax(rectMax.x - 12, rectMax.y - 8);

                drawList->AddRectFilled(fileMin, fileMax, IM_COL32(80, 150, 255, 255), 4.0f);
                drawList->AddRect(fileMin, fileMax, IM_COL32(180, 220, 255, 255), 4.0f);
            }

            if (clicked) {
                // handle click
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
                {
                    CurrentPath = entry.path();
                }
                else
                {
                    if(extension == ".scene")
                    {
                        auto entryPath = entry.path();
                        SceneManager::LoadScene(entryPath);
                    }
                }
            }

            // If it's a folder and being hovered, remember it
            if (isFolder && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            {
                HoveredDropTargetFolder = entry.path();
            }
            
            const std::string itemName = entry.path().filename().string();
            
            // Drag and drop
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::string relativePath = std::filesystem::relative(entry.path(), CONTENT_PATH).string();
                
                ImGui::SetDragDropPayload(ExtensionToAssetString(extension), relativePath.c_str(), relativePath.size() + 1);
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

            if (strlen(SearchBuffer) == 0)
            {
                for (const auto& entry : std::filesystem::directory_iterator(CurrentPath))
                {
                    RenderEntry(entry);
                    
                    // Layout in grid
                    itemIndex++;
                    if (itemIndex % columnCount != 0)
                        ImGui::SameLine();
                }
            }
            else
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(CurrentPath))
                {
                    RenderEntry(entry);
                    
                    // Layout in grid
                    itemIndex++;
                    if (itemIndex % columnCount != 0)
                        ImGui::SameLine();
                }
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
