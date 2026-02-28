#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Renderer/Model/Material.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Texture.h"
#include "Waldem/Utils/DataUtils.h"
#include "Waldem/ECS/Components/SceneEntity.h"
#include "Waldem/SceneManagement/Prefab.h"
#include "../EditorShortcuts.h"
#include <unordered_map>
#include <fstream>
#include <cstring>
#include <cctype>

namespace Waldem
{
    class ContentBrowserWidget : public IWidget
    {
    private:
        static constexpr const char* HierarchyDragPayloadType = "WALDEM_HIERARCHY_ENTITY";
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        char SearchBuffer[128] = "";
        Path CurrentPath = CONTENT_PATH;
        std::optional<Path> HoveredDropTargetFolder;
        std::optional<Path> SelectedFolderTreePath;
        std::optional<Path> SelectedAssetListPath;
        std::optional<Path> PendingRenameTarget;
        bool OpenRenamePopup = false;
        bool RenameSelectedAssetRequested = false;
        char RenameBuffer[256] = "";
        bool RenameTargetIsFile = false;
        std::string RenameTargetExtension = "";
        float CellSize = 100.0f;
        float Padding = 16.0f;
        std::unordered_map<std::string, Texture2D*> TextureThumbnails;
        inline static std::optional<Path> SharedSelectedAssetPath = std::nullopt;
        
    public:
        ContentBrowserWidget() {}

        static const std::optional<Path>& GetSelectedAssetPath()
        {
            return SharedSelectedAssetPath;
        }

        Path MakeUniqueAssetPath(const Path& folder, const std::string& baseName, const std::string& extension)
        {
            Path candidate = folder / (baseName + extension);
            int suffix = 1;

            while (std::filesystem::exists(candidate))
            {
                candidate = folder / (baseName + "_" + std::to_string(suffix) + extension);
                ++suffix;
            }

            return candidate;
        }

        std::string SanitizeAssetName(const std::string& input, const std::string& fallback = "NewPrefab") const
        {
            std::string result;
            result.reserve(input.size());

            for (char c : input)
            {
                if (std::isalnum((unsigned char)c) || c == '_' || c == '-')
                {
                    result.push_back(c);
                }
                else if (c == ' ')
                {
                    result.push_back('_');
                }
            }

            if (result.empty())
            {
                return fallback;
            }

            return result;
        }

        bool CreatePrefabAssetFromEntity(flecs::entity_t entityId, const Path& folder)
        {
            auto entity = ECS::World.entity(entityId);
            if(!entity.is_alive() || !entity.has<SceneEntity>())
            {
                return false;
            }

            Path targetFolder = folder.empty() ? CurrentPath : folder;
            std::error_code ec;
            std::filesystem::create_directories(targetFolder, ec);

            std::string entityName = entity.name().c_str();
            if(entityName.empty())
            {
                entityName = "NewPrefab";
            }

            Path prefabPath = MakeUniqueAssetPath(targetFolder, SanitizeAssetName(entityName), ".prefab");
            if(!Prefab::SaveEntityAsPrefab(entity, prefabPath))
            {
                return false;
            }

            SelectedAssetListPath = prefabPath;
            SharedSelectedAssetPath = prefabPath;
            return true;
        }

        bool CreateMaterialAsset(const Path& folder)
        {
            Path targetFolder = folder;
            if (targetFolder.empty())
            {
                targetFolder = CurrentPath;
            }

            std::error_code ec;
            std::filesystem::create_directories(targetFolder, ec);

            Path materialPath = MakeUniqueAssetPath(targetFolder, "NewMaterial", ".mat");
            std::string materialName = materialPath.stem().string();

            Material material(materialName.c_str(), TextureReference("Empty"), TextureReference("Empty"), TextureReference("Empty"));
            WDataBuffer outData;
            material.Serialize(outData);

            uint64 hash = HashFromData(outData.GetData(), outData.GetSize());
            outData.Prepend(&hash, sizeof(uint64));

            std::ofstream outFile(materialPath.c_str(), std::ios::binary | std::ios::trunc);
            if (!outFile.is_open())
            {
                return false;
            }

            outFile.write((const char*)outData.GetData(), outData.GetSize());
            outFile.close();

            SelectedAssetListPath = materialPath;
            SharedSelectedAssetPath = materialPath;
            return true;
        }

        bool IsPathInside(const Path& path, const Path& potentialParent) const
        {
            std::error_code ec;
            Path canonicalPath = std::filesystem::weakly_canonical(path, ec);
            if (ec)
            {
                canonicalPath = path;
            }

            ec.clear();
            Path canonicalParent = std::filesystem::weakly_canonical(potentialParent, ec);
            if (ec)
            {
                canonicalParent = potentialParent;
            }

            auto pathIt = canonicalPath.begin();
            auto parentIt = canonicalParent.begin();
            for (; parentIt != canonicalParent.end(); ++parentIt, ++pathIt)
            {
                if (pathIt == canonicalPath.end() || *pathIt != *parentIt)
                {
                    return false;
                }
            }

            return true;
        }

        void BeginRenamePath(const Path& targetPath)
        {
            if (targetPath.empty() || !std::filesystem::exists(targetPath))
            {
                return;
            }

            PendingRenameTarget = targetPath;
            RenameTargetIsFile = std::filesystem::is_regular_file(targetPath);
            RenameTargetExtension = RenameTargetIsFile ? targetPath.extension().string() : "";

            std::string initialName = RenameTargetIsFile ? targetPath.stem().string() : targetPath.filename().string();
            memset(RenameBuffer, 0, sizeof(RenameBuffer));
            strncpy_s(RenameBuffer, initialName.c_str(), sizeof(RenameBuffer) - 1);

            OpenRenamePopup = true;
        }

        bool CommitRename()
        {
            if (!PendingRenameTarget.has_value())
            {
                return false;
            }

            Path oldPath = PendingRenameTarget.value();
            if (!std::filesystem::exists(oldPath))
            {
                PendingRenameTarget.reset();
                return false;
            }

            std::string newName = RenameBuffer;
            if (newName.empty())
            {
                return false;
            }

            Path newPath = oldPath.parent_path() / newName;
            if (RenameTargetIsFile)
            {
                newPath += RenameTargetExtension;
            }

            if (newPath == oldPath || std::filesystem::exists(newPath))
            {
                return false;
            }

            std::error_code ec;
            std::filesystem::rename(oldPath, newPath, ec);
            if (ec)
            {
                return false;
            }

            if (SelectedAssetListPath.has_value() && SelectedAssetListPath.value() == oldPath)
            {
                SelectedAssetListPath = newPath;
            }

            if (SharedSelectedAssetPath.has_value() && SharedSelectedAssetPath.value() == oldPath)
            {
                SharedSelectedAssetPath = newPath;
            }

            if (SelectedFolderTreePath.has_value() && SelectedFolderTreePath.value() == oldPath)
            {
                SelectedFolderTreePath = newPath;
            }

            if (CurrentPath == oldPath)
            {
                CurrentPath = newPath;
            }
            else if (!RenameTargetIsFile && IsPathInside(CurrentPath, oldPath))
            {
                auto relative = std::filesystem::relative(CurrentPath, oldPath, ec);
                if (!ec)
                {
                    CurrentPath = newPath / relative;
                }
            }

            for (auto& pair : TextureThumbnails)
            {
                if (pair.second)
                {
                    Renderer::Destroy(pair.second);
                }
            }
            TextureThumbnails.clear();
            PendingRenameTarget.reset();
            return true;
        }

        void DrawRenamePopup()
        {
            if (OpenRenamePopup)
            {
                ImGui::OpenPopup("RenameAssetOrFolderPopup");
                OpenRenamePopup = false;
            }

            if (ImGui::BeginPopupModal("RenameAssetOrFolderPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted(RenameTargetIsFile ? "Rename Asset" : "Rename Folder");
                ImGui::Separator();
                ImGui::InputText("Name", RenameBuffer, IM_ARRAYSIZE(RenameBuffer), ImGuiInputTextFlags_AutoSelectAll);
                if (RenameTargetIsFile && !RenameTargetExtension.empty())
                {
                    ImGui::Text("Extension: %s", RenameTargetExtension.c_str());
                }

                if (ImGui::Button("Rename"))
                {
                    if (CommitRename())
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    PendingRenameTarget.reset();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        void Initialize(InputManager* inputManager) override
        {
            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::RenameEntity);
            }, [&]
            {
                RenameSelectedAssetRequested = true;
            });

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

        void Deinitialize() override
        {
            for (auto& pair : TextureThumbnails)
            {
                if (pair.second)
                {
                    Renderer::Destroy(pair.second);
                }
            }
            TextureThumbnails.clear();
        }

        Texture2D* GetTextureThumbnail(const Path& assetPath)
        {
            std::string key = assetPath.string();
            auto it = TextureThumbnails.find(key);
            if (it != TextureThumbnails.end())
            {
                return it->second;
            }

            auto textureDesc = CContentManager::LoadAsset<TextureDesc>(assetPath);
            if (!textureDesc)
            {
                TextureThumbnails[key] = nullptr;
                return nullptr;
            }

            Texture2D* thumbnail = Renderer::CreateTexture2D(
                "Thumbnail_" + textureDesc->Name.ToString(),
                textureDesc->Width,
                textureDesc->Height,
                textureDesc->Format,
                textureDesc->Data
            );
            
            delete textureDesc;
            TextureThumbnails[key] = thumbnail;
            return thumbnail;
        }

        void RenderFolderTreeNode(const Path& path)
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (!entry.is_directory())
                {
                    continue;
                }

                const std::string folderName = entry.path().filename().string() + "/";
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                           ImGuiTreeNodeFlags_SpanAvailWidth;

                if (SelectedFolderTreePath.has_value() && SelectedFolderTreePath.value() == entry.path())
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                bool hasChildren = false;
                for (const auto& sub : std::filesystem::directory_iterator(entry.path()))
                {
                    if (sub.is_directory()) { hasChildren = true; break; }
                }

                if (!hasChildren)
                {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

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

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::BeginMenu("Create"))
                        {
                            if (ImGui::MenuItem("Material"))
                            {
                                CreateMaterialAsset(entry.path());
                            }
                            ImGui::EndMenu();
                        }

                        if (ImGui::MenuItem("Delete"))
                        {
                            std::filesystem::remove_all(entry.path());
                        }
                        if (ImGui::MenuItem("Rename"))
                        {
                            BeginRenamePath(entry.path());
                        }
                        ImGui::EndPopup();
                }

                if (isOpen && hasChildren)
                {
                    RenderFolderTreeNode(entry.path());
                    ImGui::TreePop();
                }
            }
        }

        void RenderFolderTree(const Path& path)
        {
            RenderFolderTreeNode(path);

            // Reset selection if clicking on empty space
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && 
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
                !ImGui::IsAnyItemHovered())
            {
                SelectedFolderTreePath.reset();
                CurrentPath = CONTENT_PATH;
            }
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
                Texture2D* thumbnail = nullptr;
                if (extension == ".img")
                {
                    thumbnail = GetTextureThumbnail(entry.path());
                }

                ImVec2 fileMin(rectMin.x + 12, rectMin.y + 8);
                ImVec2 fileMax(rectMax.x - 12, rectMax.y - 8);
                if (thumbnail)
                {
                    drawList->AddImage(thumbnail->GetGPUAddress(), fileMin, fileMax);
                    drawList->AddRect(fileMin, fileMax, IM_COL32(180, 220, 255, 210), 4.0f);
                }
                else
                {
                    // Fallback file rectangle (blue)
                    drawList->AddRectFilled(fileMin, fileMax, IM_COL32(80, 150, 255, 255), 4.0f);
                    drawList->AddRect(fileMin, fileMax, IM_COL32(180, 220, 255, 255), 4.0f);
                }
            }

            if (clicked) {
                // handle click
            }

            // --- Selection on single click ---
            if (ImGui::IsItemClicked())
            {
                SelectedAssetListPath = entry.path(); // Works for both files and folders
                SharedSelectedAssetPath = entry.path();
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

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                {
                    const auto entityId = *(const flecs::entity_t*)payload->Data;
                    Path targetFolder = isFolder ? entry.path() : CurrentPath;
                    CreatePrefabAssetFromEntity(entityId, targetFolder);
                }
                ImGui::EndDragDropTarget();
            }
            
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
                if (isFolder && ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("Material"))
                    {
                        CreateMaterialAsset(entry.path());
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Delete"))
                    std::filesystem::remove_all(entry.path());
                if (ImGui::MenuItem("Rename"))
                    BeginRenamePath(entry.path());
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
                if (ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("Material"))
                    {
                        CreateMaterialAsset(CurrentPath);
                    }
                    if (ImGui::MenuItem("Folder"))
                    {
                        openCreateNewFolderPopup = true;
                    }
                    ImGui::EndMenu();
                }

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
                SharedSelectedAssetPath.reset();
            }

            float dropTargetHeight = ImGui::GetContentRegionAvail().y;
            if(dropTargetHeight < 24.0f)
            {
                dropTargetHeight = 24.0f;
            }

            ImGui::InvisibleButton("##PrefabDropTarget", ImVec2(ImGui::GetContentRegionAvail().x, dropTargetHeight));
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                {
                    const auto entityId = *(const flecs::entity_t*)payload->Data;
                    Path targetFolder = HoveredDropTargetFolder.has_value() ? HoveredDropTargetFolder.value() : CurrentPath;
                    CreatePrefabAssetFromEntity(entityId, targetFolder);
                }
                ImGui::EndDragDropTarget();
            }
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                HoveredDropTargetFolder.reset();
                
                //search bar
                if (ImGui::Button("Create"))
                {
                    ImGui::OpenPopup("ContentCreatePopup");
                }
                if (ImGui::BeginPopup("ContentCreatePopup"))
                {
                    if (ImGui::MenuItem("Material"))
                    {
                        CreateMaterialAsset(CurrentPath);
                    }
                    ImGui::EndPopup();
                }

                ImGui::SameLine();
                ImGui::InputTextWithHint("##Search", "Search...", SearchBuffer, IM_ARRAYSIZE(SearchBuffer));
                ImGui::Separator();

                //folders tree
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
                ImGui::BeginChild("FoldersTree", ImVec2(240, 0), true, ImGuiWindowFlags_None);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
                RenderFolderTree(CONTENT_PATH);
                ImGui::PopStyleVar();
                ImGui::EndChild();
                ImGui::PopStyleVar();

                ImGui::SameLine();

                //assets list
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
                ImGui::BeginChild("AssetList", ImVec2(0, 0), true, ImGuiWindowFlags_None);
                RenderAssetsList();
                ImGui::EndChild();
                ImGui::PopStyleVar();

                const bool contentFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
                if (RenameSelectedAssetRequested && contentFocused)
                {
                    if (SelectedAssetListPath.has_value())
                    {
                        BeginRenamePath(SelectedAssetListPath.value());
                    }
                    else if (SelectedFolderTreePath.has_value())
                    {
                        BeginRenamePath(SelectedFolderTreePath.value());
                    }
                }

                DrawRenamePopup();
            }

            RenameSelectedAssetRequested = false;
            ImGui::End();
        }
    };
}
