#pragma once
#include "imgui.h"
#include "Widget.h"
#include "../EditorShortcutContext.h"
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Renderer/Model/Material.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Texture.h"
#include "Waldem/Utils/DataUtils.h"
#include "Waldem/ECS/Components/SceneEntity.h"
#include "Waldem/SceneManagement/Prefab.h"
#include "Waldem/Scripting/ScriptEngine.h"
#include "../EditorShortcuts.h"
#include <unordered_map>
#include <fstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <future>
#include <chrono>
#include <vector>
#include <regex>
#include <cstdlib>

#include "imgui_internal.h"
#include "Waldem/ProjectManagement/ProjectManager.h"

namespace Waldem
{
    class ContentBrowserWidget : public IWidget
    {
    private:
        static constexpr const char* HierarchyDragPayloadType = "WALDEM_HIERARCHY_ENTITY";
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        char SearchBuffer[128] = "";
        Path CurrentPath = PROJECT_CONTENT_PATH;
        std::optional<Path> HoveredDropTargetFolder;
        std::optional<Path> SelectedFolderTreePath;
        std::optional<Path> SelectedAssetListPath;
        std::optional<Path> PendingRenameTarget;
        bool OpenRenamePopup = false;
        bool FocusRenameInput = false;
        bool RenameSelectedAssetRequested = false;
        bool DeleteSelectedAssetRequested = false;
        bool OpenModelImportPopup = false;
        bool ModelImportPending = false;
        bool ImportTaskRunning = false;
        Path PendingModelImportSourcePath;
        Path PendingModelImportTargetPath = PROJECT_CONTENT_PATH;
        float PendingModelImportScale = 1.0f;
        Path LastImportTargetPath;
        std::future<bool> ImportTask;
        char RenameBuffer[256] = "";
        bool RenameTargetIsFile = false;
        std::string RenameTargetExtension = "";
        float CellSize = 100.0f;
        float Padding = 16.0f;
        std::unordered_map<std::string, Texture2D*> TextureThumbnails;
        int ThumbnailsCreatedThisFrame = 0;
        static constexpr int ThumbnailCreateBudgetPerFrame = 4;
        static constexpr int ThumbnailMaxDimension = 128;
        struct AssetListEntry
        {
            Path AssetPath;
            bool IsDirectory = false;
            WString Extension;
        };
        std::vector<AssetListEntry> CachedAssetEntries;
        Path CachedEntriesPath;
        std::string CachedSearchQuery;
        bool AssetEntriesDirty = true;
        Path LastProjectContentRoot;
        inline static std::optional<Path> SharedSelectedAssetPath = std::nullopt;
        inline static std::optional<Path> SharedFocusAssetPath = std::nullopt;
        std::optional<Path> PendingScrollToAssetPath = std::nullopt;
        
    public:
        ContentBrowserWidget() {}

        static const std::optional<Path>& GetSelectedAssetPath()
        {
            return SharedSelectedAssetPath;
        }

        static void FocusAssetPath(const Path& assetPath)
        {
            SharedFocusAssetPath = assetPath;
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

        bool CreatePrefabAssetFromEntity(ECS::EntityT entityId, const Path& folder)
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
            PendingScrollToAssetPath = prefabPath;
            InvalidateAssetEntries();
            BeginRenamePath(prefabPath);
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
            PendingScrollToAssetPath = materialPath;
            InvalidateAssetEntries();
            BeginRenamePath(materialPath);
            return true;
        }

        bool CreateScriptAsset(const Path& folder)
        {
            Path targetFolder = folder;
            if (targetFolder.empty())
            {
                targetFolder = CurrentPath;
            }

            std::error_code ec;
            std::filesystem::create_directories(targetFolder, ec);

            Path scriptPath = MakeUniqueAssetPath(targetFolder, "NewScript", ".cs");
            std::string scriptName = scriptPath.stem().string();

            std::ofstream outFile(scriptPath.c_str(), std::ios::binary | std::ios::trunc);
            if (!outFile.is_open())
            {
                return false;
            }

            const std::string defaultScriptCode =
                "using Waldem;\n\n"
                "namespace Waldem\n"
                "{\n"
                "    public class " + scriptName + " : ScriptableEntity\n"
                "    {\n"
                "        public Vector3 SomeVectorField = new Vector3(1, 2, 3);\n\n"
                "        protected override void OnUpdate(float dt)\n"
                "        {\n"
                "            Debug.Log(\"Updating!\");\n"
                "        }\n"
                "    }\n"
                "}\n"; 

            outFile.write(defaultScriptCode.c_str(), (std::streamsize)defaultScriptCode.size());
            outFile.close();

            SelectedAssetListPath = scriptPath;
            SharedSelectedAssetPath = scriptPath;
            PendingScrollToAssetPath = scriptPath;
            InvalidateAssetEntries();
            BeginRenamePath(scriptPath);
            if(ProjectManager::GenerateProjectFiles() && ScriptEngine::IsInitialized())
            {
                ScriptEngine::ReloadScripts(true);
            }
            return true;
        }

        void StartImportTask(const Path& sourcePath, const Path& targetPath)
        {
            if (ImportTaskRunning)
            {
                return;
            }

            Path sourceCopy = sourcePath;
            Path targetCopy = targetPath;
            LastImportTargetPath = targetCopy;
            ImportTaskRunning = true;
            ImportTask = std::async(std::launch::async, [sourceCopy, targetCopy]() mutable
            {
                return CContentManager::ImportTo(sourceCopy, targetCopy);
            });
        }

        void StartImportTask(const Path& sourcePath, const Path& targetPath, const ModelImportSettings& settings)
        {
            if (ImportTaskRunning)
            {
                return;
            }

            Path sourceCopy = sourcePath;
            Path targetCopy = targetPath;
            ModelImportSettings settingsCopy = settings;
            LastImportTargetPath = targetCopy;
            ImportTaskRunning = true;
            ImportTask = std::async(std::launch::async, [sourceCopy, targetCopy, settingsCopy]() mutable
            {
                return CContentManager::ImportTo(sourceCopy, targetCopy, settingsCopy);
            });
        }

        void PollImportTask()
        {
            if (!ImportTaskRunning)
            {
                return;
            }

            if (ImportTask.valid() && ImportTask.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                const bool success = ImportTask.get();
                ImportTaskRunning = false;
                if (success)
                {
                    SelectedAssetListPath = LastImportTargetPath;
                    SharedSelectedAssetPath = LastImportTargetPath;
                    InvalidateAssetEntries();
                }
            }
        }

        Path ResolveFocusAssetPath(const Path& inPath) const
        {
            if (inPath.empty() || inPath == "Empty")
            {
                return {};
            }

            auto makeAbsolute = [](const Path& pathIn)
            {
                if (pathIn.is_absolute())
                {
                    return pathIn;
                }
                return Path(PROJECT_CONTENT_PATH) / pathIn;
            };

            Path candidate = makeAbsolute(inPath);
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }

            if (!candidate.has_extension())
            {
                static const char* extensions[] = { ".mesh", ".mat", ".img", ".wav", ".scene", ".prefab", ".model", ".cs" };
                for (const auto* extension : extensions)
                {
                    Path withExtension = candidate;
                    withExtension.replace_extension(extension);
                    if (std::filesystem::exists(withExtension))
                    {
                        return withExtension;
                    }
                }
            }

            return {};
        }

        bool IsPathInsideScripts(const Path& path) const
        {
            if(!ProjectManager::HasProject())
            {
                return false;
            }

            const Path scriptsPath = ProjectManager::CurrentProject.GetScriptsPath();
            return path == scriptsPath || IsPathInside(path, scriptsPath);
        }

        void RefreshScriptsAfterAssetChange(const Path& path)
        {
            if(!IsPathInsideScripts(path))
            {
                return;
            }

            if(ProjectManager::GenerateProjectFiles() && ScriptEngine::IsInitialized())
            {
                ScriptEngine::ReloadScripts(true);
            }
        }

        void UpdateScriptClassNameIfNeeded(const Path& oldPath, const Path& newPath)
        {
            if(oldPath.extension() != ".cs" || newPath.extension() != ".cs")
            {
                return;
            }

            const std::string oldName = oldPath.stem().string();
            const std::string newName = newPath.stem().string();
            if(oldName.empty() || newName.empty() || oldName == newName)
            {
                return;
            }

            std::ifstream inFile(newPath);
            if(!inFile.is_open())
            {
                return;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string source = buffer.str();
            inFile.close();

            const std::regex classPattern("\\bclass\\s+" + oldName + "\\b");
            const std::string updated = std::regex_replace(source, classPattern, "class " + newName, std::regex_constants::format_first_only);
            if(updated == source)
            {
                return;
            }

            std::ofstream outFile(newPath, std::ios::trunc);
            if(!outFile.is_open())
            {
                return;
            }

            outFile << updated;
        }

        Path FindRiderExecutable() const
        {
            static Path cachedPath;
            if(!cachedPath.empty())
            {
                return cachedPath;
            }

            const char* pathEnv = std::getenv("PATH");
            if(pathEnv != nullptr)
            {
                std::stringstream pathStream(pathEnv);
                std::string segment;
                while(std::getline(pathStream, segment, ';'))
                {
                    if(segment.empty())
                    {
                        continue;
                    }

                    Path riderExe = Path(segment) / "rider64.exe";
                    if(std::filesystem::exists(riderExe))
                    {
                        cachedPath = riderExe;
                        return cachedPath;
                    }

                    Path riderBat = Path(segment) / "rider.bat";
                    if(std::filesystem::exists(riderBat))
                    {
                        cachedPath = riderBat;
                        return cachedPath;
                    }
                }
            }

            const Path jetBrainsRoot = "C:/Program Files/JetBrains";
            std::error_code ec;
            if(std::filesystem::exists(jetBrainsRoot))
            {
                for(const auto& entry : std::filesystem::recursive_directory_iterator(jetBrainsRoot, std::filesystem::directory_options::skip_permission_denied, ec))
                {
                    if(ec)
                    {
                        break;
                    }

                    if(!entry.is_regular_file())
                    {
                        continue;
                    }

                    const std::string filename = ToLower(entry.path().filename().string());
                    if(filename == "rider64.exe" || filename == "rider.bat")
                    {
                        cachedPath = entry.path();
                        return cachedPath;
                    }
                }
            }

            return {};
        }

        bool OpenScriptInRider(const Path& scriptPath) const
        {
            const Path riderExecutable = FindRiderExecutable();
            if(riderExecutable.empty())
            {
                return false;
            }

            Path projectToOpen;
            Path current = GetCurrentFolder();
            for(int i = 0; i < 8 && !current.empty(); ++i)
            {
                const Path solutionPath = current / "Waldem.sln";
                if(std::filesystem::exists(solutionPath))
                {
                    projectToOpen = solutionPath;
                    break;
                }

                current = current.parent_path();
            }

            std::wstring commandLine = L"\"" + riderExecutable.wstring() + L"\"";
            if(!projectToOpen.empty())
            {
                commandLine += L" \"" + projectToOpen.wstring() + L"\"";
            }
            commandLine += L" \"" + std::filesystem::absolute(scriptPath).wstring() + L"\"";

            STARTUPINFOW startupInfo = {};
            startupInfo.cb = sizeof(startupInfo);
            PROCESS_INFORMATION processInfo = {};

            std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
            mutableCommandLine.push_back(L'\0');

            if(!CreateProcessW(
                nullptr,
                mutableCommandLine.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &startupInfo,
                &processInfo))
            {
                return false;
            }

            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            return true;
        }

        void ProcessFocusRequest()
        {
            if (!SharedFocusAssetPath.has_value())
            {
                return;
            }

            const Path resolvedPath = ResolveFocusAssetPath(SharedFocusAssetPath.value());
            SharedFocusAssetPath.reset();
            if (resolvedPath.empty())
            {
                return;
            }

            CurrentPath = resolvedPath.parent_path();
            SelectedFolderTreePath = CurrentPath;
            SelectedAssetListPath = resolvedPath;
            SharedSelectedAssetPath = resolvedPath;
            PendingScrollToAssetPath = resolvedPath;
            InvalidateAssetEntries();
            ImGui::SetWindowFocus("Content");
        }

        void DeleteSelectedAssetOrFolder()
        {
            if (!SelectedAssetListPath.has_value())
            {
                return;
            }

            const Path targetPath = SelectedAssetListPath.value();
            const bool shouldRefreshScripts = IsPathInsideScripts(targetPath);
            std::error_code ec;
            if (std::filesystem::is_directory(targetPath))
            {
                std::filesystem::remove_all(targetPath, ec);
            }
            else
            {
                std::filesystem::remove(targetPath, ec);
            }

            if (!ec)
            {
                SelectedAssetListPath.reset();
                SharedSelectedAssetPath.reset();

                for (auto& pair : TextureThumbnails)
                {
                    if (pair.second)
                    {
                        Renderer::Destroy(pair.second);
                    }
                }
                TextureThumbnails.clear();
                InvalidateAssetEntries();
                if(shouldRefreshScripts)
                {
                    RefreshScriptsAfterAssetChange(targetPath);
                }
            }
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

            FocusRenameInput = true;
            OpenRenamePopup = targetPath.parent_path() != CurrentPath;
        }

        std::string GetDisplayNameForEntry(const Path& entryPath, bool isFolder) const
        {
            if (isFolder)
            {
                return entryPath.filename().string();
            }

            if (entryPath.extension() == ".cs")
            {
                return entryPath.stem().string();
            }

            return entryPath.filename().string();
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
            FocusRenameInput = false;
            UpdateScriptClassNameIfNeeded(oldPath, newPath);
            InvalidateAssetEntries();
            RefreshScriptsAfterAssetChange(oldPath);
            RefreshScriptsAfterAssetChange(newPath);
            return true;
        }

        void DrawRenamePopup()
        {
            if(PendingRenameTarget.has_value() && PendingRenameTarget->parent_path() == CurrentPath)
            {
                OpenRenamePopup = false;
            }

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
                    FocusRenameInput = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        bool IsModelImportSource(const Path& path) const
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){ return std::tolower(c); });
            return extension == ".fbx" || extension == ".gltf" || extension == ".glb";
        }

        void QueueModelImport(const Path& sourcePath, const Path& targetPath)
        {
            PendingModelImportSourcePath = sourcePath;
            PendingModelImportTargetPath = targetPath.empty() ? CurrentPath : targetPath;
            PendingModelImportScale = 1.0f;
            ModelImportPending = true;
            OpenModelImportPopup = true;
        }

        void DrawModelImportPopup()
        {
            if (OpenModelImportPopup)
            {
                ImGui::OpenPopup("Model Import Settings");
                OpenModelImportPopup = false;
            }

            if (!ModelImportPending)
            {
                return;
            }

            if (ImGui::BeginPopupModal("Model Import Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted("Import model");
                ImGui::Separator();
                ImGui::Text("Source: %s", PendingModelImportSourcePath.filename().string().c_str());
                ImGui::InputFloat("Scale", &PendingModelImportScale, 0.01f, 0.1f, "%.3f");
                if (PendingModelImportScale <= 0.0f)
                {
                    PendingModelImportScale = 0.001f;
                }

                if (ImGui::Button("Import"))
                {
                    Path target = PendingModelImportTargetPath;
                    ModelImportSettings settings{};
                    settings.UniformScale = PendingModelImportScale;
                    StartImportTask(PendingModelImportSourcePath, target, settings);

                    ModelImportPending = false;
                    PendingModelImportSourcePath.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    ModelImportPending = false;
                    PendingModelImportSourcePath.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        void Initialize(InputManager* inputManager) override
        {
            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::RenameEntity, [&]
            {
                RenameSelectedAssetRequested = true;
            }, [] { return EditorShortcutContexts::Has(EditorShortcutContext::ContentBrowser); });

            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::DeleteEntity, [&]
            {
                DeleteSelectedAssetRequested = true;
            }, [] { return EditorShortcutContexts::Has(EditorShortcutContext::ContentBrowser); });

            CContentManager::SubscribeToFileDroppedEvent([this](Path path)
            {
                Path target = HoveredDropTargetFolder.has_value() ? HoveredDropTargetFolder.value() : CurrentPath;
                
                if(!target.empty())
                {
                    if (IsModelImportSource(path))
                    {
                        QueueModelImport(path, target);
                    }
                    else
                    {
                        StartImportTask(path, target);
                    }
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

            if (ThumbnailsCreatedThisFrame >= ThumbnailCreateBudgetPerFrame)
            {
                return nullptr;
            }

            auto textureDesc = CContentManager::LoadAsset<TextureDesc>(assetPath);
            if (!textureDesc)
            {
                TextureThumbnails[key] = nullptr;
                return nullptr;
            }

            int thumbnailWidth = textureDesc->Width;
            int thumbnailHeight = textureDesc->Height;
            uint8* thumbnailData = textureDesc->Data;
            bool ownsThumbnailData = false;

            const bool canDownsampleRGBA =
                textureDesc->Format == TextureFormat::R8G8B8A8_UNORM ||
                textureDesc->Format == TextureFormat::R8G8B8A8_UNORM_SRGB ||
                textureDesc->Format == TextureFormat::B8G8R8A8_UNORM ||
                textureDesc->Format == TextureFormat::B8G8R8A8_UNORM_SRGB;

            if (canDownsampleRGBA &&
                textureDesc->Data &&
                textureDesc->Width > 0 &&
                textureDesc->Height > 0 &&
                (textureDesc->Width > ThumbnailMaxDimension || textureDesc->Height > ThumbnailMaxDimension))
            {
                const float scale = std::min(
                    (float)ThumbnailMaxDimension / (float)textureDesc->Width,
                    (float)ThumbnailMaxDimension / (float)textureDesc->Height
                );

                thumbnailWidth = std::max(1, (int)(textureDesc->Width * scale));
                thumbnailHeight = std::max(1, (int)(textureDesc->Height * scale));
                thumbnailData = new uint8[thumbnailWidth * thumbnailHeight * 4];
                ownsThumbnailData = true;

                for (int y = 0; y < thumbnailHeight; ++y)
                {
                    const int srcY = (y * textureDesc->Height) / thumbnailHeight;
                    for (int x = 0; x < thumbnailWidth; ++x)
                    {
                        const int srcX = (x * textureDesc->Width) / thumbnailWidth;
                        const int srcIndex = (srcY * textureDesc->Width + srcX) * 4;
                        const int dstIndex = (y * thumbnailWidth + x) * 4;

                        thumbnailData[dstIndex + 0] = textureDesc->Data[srcIndex + 0];
                        thumbnailData[dstIndex + 1] = textureDesc->Data[srcIndex + 1];
                        thumbnailData[dstIndex + 2] = textureDesc->Data[srcIndex + 2];
                        thumbnailData[dstIndex + 3] = textureDesc->Data[srcIndex + 3];
                    }
                }
            }

            Texture2D* thumbnail = Renderer::CreateTexture2D(
                "Thumbnail_" + textureDesc->Name.ToString(),
                thumbnailWidth,
                thumbnailHeight,
                textureDesc->Format,
                thumbnailData
            );

            if (ownsThumbnailData)
            {
                delete[] thumbnailData;
            }
            
            delete textureDesc;
            TextureThumbnails[key] = thumbnail;
            ThumbnailsCreatedThisFrame++;
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
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

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
                    InvalidateAssetEntries();
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
                            if (ImGui::MenuItem("Script"))
                            {
                                CreateScriptAsset(entry.path());
                            }
                            ImGui::EndMenu();
                        }

                        if (ImGui::MenuItem("Delete"))
                        {
                            std::filesystem::remove_all(entry.path());
                            InvalidateAssetEntries();
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
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
            {
                SelectedFolderTreePath.reset();
                CurrentPath = PROJECT_CONTENT_PATH;
                InvalidateAssetEntries();
            }
        }


        std::string ToLower(const std::string& str) const
        {
            std::string result = str;
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::tolower(c); });
            return result;
        }

        void InvalidateAssetEntries()
        {
            AssetEntriesDirty = true;
        }

        void RebuildAssetEntriesIfNeeded()
        {
            const std::string searchLower = ToLower(SearchBuffer);
            if (!AssetEntriesDirty && CachedEntriesPath == CurrentPath && CachedSearchQuery == searchLower)
            {
                return;
            }

            CachedAssetEntries.clear();
            std::error_code ec;
            const auto options = std::filesystem::directory_options::skip_permission_denied;

            if (searchLower.empty())
            {
                for (const auto& entry : std::filesystem::directory_iterator(CurrentPath, options, ec))
                {
                    std::error_code isDirEc;
                    const bool isDirectory = entry.is_directory(isDirEc);
                    if (isDirEc)
                    {
                        continue;
                    }

                    const WString extension = entry.path().extension().string();
                    if (!isDirectory && !IsSupportedExtension(extension))
                    {
                        continue;
                    }

                    CachedAssetEntries.push_back({ entry.path(), isDirectory, extension });
                }
            }
            else
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(CurrentPath, options, ec))
                {
                    std::error_code isDirEc;
                    const bool isDirectory = entry.is_directory(isDirEc);
                    if (isDirEc)
                    {
                        continue;
                    }

                    const WString extension = entry.path().extension().string();
                    if (!isDirectory && !IsSupportedExtension(extension))
                    {
                        continue;
                    }

                    std::error_code relEc;
                    const std::string relativePath = std::filesystem::relative(entry.path(), PROJECT_CONTENT_PATH, relEc).string();
                    if (!relEc)
                    {
                        if (ToLower(relativePath).find(searchLower) == std::string::npos)
                        {
                            continue;
                        }
                    }

                    CachedAssetEntries.push_back({ entry.path(), isDirectory, extension });
                }
            }

            std::sort(CachedAssetEntries.begin(), CachedAssetEntries.end(), [this](const AssetListEntry& lhs, const AssetListEntry& rhs)
            {
                if (lhs.IsDirectory != rhs.IsDirectory)
                {
                    return lhs.IsDirectory && !rhs.IsDirectory;
                }

                return ToLower(lhs.AssetPath.filename().string()) < ToLower(rhs.AssetPath.filename().string());
            });

            CachedEntriesPath = CurrentPath;
            CachedSearchQuery = searchLower;
            AssetEntriesDirty = false;
        }

        void RenderEntry(const AssetListEntry& entry)
        {
            const Path& entryPath = entry.AssetPath;
            const bool isFolder = entry.IsDirectory;
            const WString& extension = entry.Extension;

            ImGui::BeginGroup();

            std::string id = "##Icon_" + entryPath.string();
            ImVec2 size(CellSize, CellSize);
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImVec2 rectMin = cursor;
            ImVec2 rectMax = ImVec2(cursor.x + size.x, cursor.y + size.y);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(rectMin, rectMax, IM_COL32(50, 50, 50, 255), 6.0f);
            ImGui::InvisibleButton(id.c_str(), size);

            if (isFolder)
            {
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
                    thumbnail = GetTextureThumbnail(entryPath);
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
                    drawList->AddRectFilled(fileMin, fileMax, IM_COL32(80, 150, 255, 255), 4.0f);
                    drawList->AddRect(fileMin, fileMax, IM_COL32(180, 220, 255, 255), 4.0f);
                }
            }

            if (ImGui::IsItemClicked())
            {
                SelectedAssetListPath = entryPath;
                SharedSelectedAssetPath = entryPath;
            }

            if (SelectedAssetListPath.has_value() && SelectedAssetListPath.value() == entryPath)
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 255, 0, 255), 4.0f
                );
            }

            if (PendingScrollToAssetPath.has_value() && PendingScrollToAssetPath.value() == entryPath)
            {
                ImGui::SetScrollHereY(0.5f);
                PendingScrollToAssetPath.reset();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (isFolder)
                {
                    CurrentPath = entryPath;
                    InvalidateAssetEntries();
                }
                else if (extension == ".scene")
                {
                    Path scenePath = entryPath;
                    SceneManager::LoadScene(scenePath);
                }
                else if (extension == ".cs")
                {
                    OpenScriptInRider(entryPath);
                }
            }

            if (isFolder && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            {
                HoveredDropTargetFolder = entryPath;
            }

            const std::string itemName = entryPath.filename().string();
            const std::string displayBaseName = GetDisplayNameForEntry(entryPath, isFolder);

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                {
                    const auto entityId = *(const ECS::EntityT*)payload->Data;
                    Path targetFolder = isFolder ? entryPath : CurrentPath;
                    CreatePrefabAssetFromEntity(entityId, targetFolder);
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::string relativePath = std::filesystem::relative(entryPath, PROJECT_CONTENT_PATH).string();
                ImGui::SetDragDropPayload(ExtensionToAssetString(extension), relativePath.c_str(), relativePath.size() + 1);
                ImGui::Text("%s", itemName.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginPopupContextItem())
            {
                SelectedAssetListPath = entryPath;
                SharedSelectedAssetPath = entryPath;

                if (isFolder && ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("Material"))
                    {
                        CreateMaterialAsset(entryPath);
                    }
                    if (ImGui::MenuItem("Script"))
                    {
                        CreateScriptAsset(entryPath);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Delete"))
                {
                    std::error_code ec;
                    if (isFolder)
                    {
                        std::filesystem::remove_all(entryPath, ec);
                    }
                    else
                    {
                        std::filesystem::remove(entryPath, ec);
                    }
                    InvalidateAssetEntries();
                }
                if (ImGui::MenuItem("Rename"))
                {
                    BeginRenamePath(entryPath);
                }
                ImGui::EndPopup();
            }

            const bool isRenamingThisEntry = PendingRenameTarget.has_value() && PendingRenameTarget.value() == entryPath && !OpenRenamePopup;
            if (isRenamingThisEntry)
            {
                ImGui::SetNextItemWidth(CellSize);
                if (FocusRenameInput)
                {
                    ImGui::SetKeyboardFocusHere();
                    FocusRenameInput = false;
                }

                const std::string renameId = "##Rename_" + entryPath.string();
                const bool submit = ImGui::InputText(renameId.c_str(), RenameBuffer, IM_ARRAYSIZE(RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                const bool deactivate = ImGui::IsItemDeactivatedAfterEdit();
                if (submit || deactivate)
                {
                    CommitRename();
                }
            }
            else
            {
                std::string displayName = displayBaseName;
                if (ImGui::CalcTextSize(displayName.c_str()).x > CellSize)
                {
                    const char* ellipsis = "...";
                    size_t len = displayName.size();
                    while (len > 0)
                    {
                        std::string candidate = displayName.substr(0, len) + ellipsis;
                        if (ImGui::CalcTextSize(candidate.c_str()).x <= CellSize)
                        {
                            displayName = candidate;
                            break;
                        }
                        --len;
                    }
                }
                ImGui::TextUnformatted(displayName.c_str());
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayShort))
            {
                ImGui::SetTooltip("%s", displayBaseName.c_str());
            }

            ImGui::EndGroup();
        }

        void RenderAssetsList()
        {
            const bool showParentEntry = (CurrentPath != PROJECT_CONTENT_PATH);
            if (showParentEntry)
            {
                ImGui::Selectable("..");

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    CurrentPath = CurrentPath.parent_path();
                    InvalidateAssetEntries();
                }

                // Keep parent-navigation row visually separate from the asset grid.
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            static bool openCreateNewFolderPopup = false;
            
            if (ImGui::BeginPopupContextWindow("CreateNewFolderContextWindow"))
            {
                if (ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("Folder"))
                    {
                        openCreateNewFolderPopup = true;
                    }
                    if (ImGui::MenuItem("Material"))
                    {
                        CreateMaterialAsset(CurrentPath);
                    }
                    if (ImGui::MenuItem("Script"))
                    {
                        CreateScriptAsset(CurrentPath);
                    }
                    ImGui::EndMenu();
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
                        {
                            std::filesystem::create_directory(newFolderPath);
                            SelectedAssetListPath = newFolderPath;
                            SharedSelectedAssetPath = newFolderPath;
                            PendingScrollToAssetPath = newFolderPath;
                            InvalidateAssetEntries();
                            BeginRenamePath(newFolderPath);
                        }
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

            RebuildAssetEntriesIfNeeded();

            const float rowHeight = CellSize + ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
            const int totalItems = (int)CachedAssetEntries.size();
            const int rowCount = (totalItems + columnCount - 1) / columnCount;

            if (PendingScrollToAssetPath.has_value())
            {
                for (int i = 0; i < totalItems; ++i)
                {
                    if (CachedAssetEntries[i].AssetPath == PendingScrollToAssetPath.value())
                    {
                        ImGui::SetScrollY((float)(i / columnCount) * rowHeight);
                        break;
                    }
                }
            }

            ImGuiListClipper clipper;
            clipper.Begin(rowCount, rowHeight);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                {
                    const int startIndex = row * columnCount;
                    const int endIndex = std::min(startIndex + columnCount, totalItems);

                    for (int itemIndex = startIndex; itemIndex < endIndex; ++itemIndex)
                    {
                        RenderEntry(CachedAssetEntries[itemIndex]);
                        if (itemIndex + 1 < endIndex)
                        {
                            ImGui::SameLine();
                        }
                    }
                }
            }

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGui::IsAnyItemHovered())
            {
                SelectedAssetListPath.reset(); // Deselect
                SharedSelectedAssetPath.reset();
            }

            ImVec2 dropTargetSize = ImGui::GetContentRegionAvail();
            if (dropTargetSize.y < 24.0f)
            {
                dropTargetSize.y = 24.0f;
            }

            // ImGui::InvisibleButton requires both dimensions to be strictly > 0.
            dropTargetSize.x = ImMax(dropTargetSize.x, 1.0f);
            dropTargetSize.y = ImMax(dropTargetSize.y, 1.0f);
            ImGui::InvisibleButton("##PrefabDropTarget", dropTargetSize);
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                {
                    const auto entityId = *(const ECS::EntityT*)payload->Data;
                    Path targetFolder = HoveredDropTargetFolder.has_value() ? HoveredDropTargetFolder.value() : CurrentPath;
                    CreatePrefabAssetFromEntity(entityId, targetFolder);
                }
                ImGui::EndDragDropTarget();
            }
        }

        void OnDraw(float deltaTime) override
        {
            PollImportTask();
            ThumbnailsCreatedThisFrame = 0;

            if(!ProjectManager::HasProject())
            {
                LastProjectContentRoot.clear();
                CurrentPath.clear();
                PendingModelImportTargetPath.clear();
                SelectedFolderTreePath.reset();
                SelectedAssetListPath.reset();
                HoveredDropTargetFolder.reset();
                EditorShortcutContexts::SetActive(EditorShortcutContext::ContentBrowser, false);
                RenameSelectedAssetRequested = false;
                DeleteSelectedAssetRequested = false;
                return;
            }

            const Path projectContentRoot = PROJECT_CONTENT_PATH;
            if(projectContentRoot != LastProjectContentRoot || CurrentPath.empty() || !exists(CurrentPath) || !IsPathInside(CurrentPath, projectContentRoot))
            {
                LastProjectContentRoot = projectContentRoot;
                CurrentPath = projectContentRoot;
                PendingModelImportTargetPath = projectContentRoot;
                SelectedFolderTreePath.reset();
                SelectedAssetListPath.reset();
                InvalidateAssetEntries();
            }

            if(!exists(projectContentRoot))
            {
                EditorShortcutContexts::SetActive(EditorShortcutContext::ContentBrowser, false);
                RenameSelectedAssetRequested = false;
                DeleteSelectedAssetRequested = false;
                return;
            }

            const bool isVisible = ImGui::Begin("Content###Content", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            if (isVisible)
            {
                ProcessFocusRequest();
                HoveredDropTargetFolder.reset();

                float importProgress = 0.0f;
                std::string importLabel;
                if (CContentManager::GetImportStatus(importProgress, importLabel))
                {
                    std::string label = importLabel.empty() ? "Importing..." : ("Importing " + importLabel + "...");
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::ProgressBar(importProgress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                    ImGui::Separator();
                }
                
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
                    if (ImGui::MenuItem("Script"))
                    {
                        CreateScriptAsset(CurrentPath);
                    }
                    ImGui::EndPopup();
                }

                ImGui::SameLine();
                if (ImGui::InputTextWithHint("##Search", "Search...", SearchBuffer, IM_ARRAYSIZE(SearchBuffer)))
                {
                    InvalidateAssetEntries();
                }
                ImGui::Separator();

                //folders tree
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
                ImGui::BeginChild("FoldersTree", ImVec2(240, 0), true, ImGuiWindowFlags_None);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
                RenderFolderTree(projectContentRoot);
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
                EditorShortcutContexts::SetActive(EditorShortcutContext::ContentBrowser, contentFocused);
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

                if (DeleteSelectedAssetRequested && contentFocused)
                {
                    DeleteSelectedAssetOrFolder();
                }

                DrawRenamePopup();
                DrawModelImportPopup();
            }
            else
            {
                EditorShortcutContexts::SetActive(EditorShortcutContext::ContentBrowser, false);
            }

            RenameSelectedAssetRequested = false;
            DeleteSelectedAssetRequested = false;
            ImGui::End();
        }
    };
}
