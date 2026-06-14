#pragma once
#include "imgui.h"
#include "CoachWidget.h"
#include "Widget.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Editor/EditorSimulation.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Scripting/ScriptEngine.h"
#include "Waldem/ECS/Systems/CoreSystems/PhysXSystem.h"
#include "Waldem/ProjectManagement/ProjectBuilder.h"
#include "Waldem/Utils/FileUtils.h"
#include "Commands/EditorCommands.h"
#include "../EditorShortcuts.h"
#include <cstdint>
#include <unordered_map>
#include <string>

#include "CreateProjectWidget.h"

namespace Waldem
{
    class MenuBarWidget : public IWidget
    {
    private:
        inline static float SecondaryBarHeight = 0.0f;
        Path LastBuildDirectory = "";

        struct KeyOption
        {
            int Key;
            const char* Label;
        };

        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        uint64_t GetBindingKey(const ShortcutBinding& binding) const
        {
            uint64_t key = (uint64_t)(uint32)binding.Key;
            key = (key << 1) | (binding.Ctrl ? 1ull : 0ull);
            key = (key << 1) | (binding.Shift ? 1ull : 0ull);
            key = (key << 1) | (binding.Alt ? 1ull : 0ull);
            return key;
        }

        bool DrawShortcutRow(EditorShortcutAction action, bool hasConflict)
        {
            static const KeyOption keyOptions[] =
            {
                { A, "A" }, { B, "B" }, { C, "C" }, { D, "D" }, { E, "E" }, { F, "F" }, { G, "G" }, { H, "H" }, { I, "I" }, { J, "J" }, { K, "K" }, { L, "L" }, { M, "M" },
                { N, "N" }, { O, "O" }, { P, "P" }, { Q, "Q" }, { R, "R" }, { S, "S" }, { T, "T" }, { U, "U" }, { V, "V" }, { W, "W" }, { X, "X" }, { Y, "Y" }, { Z, "Z" },
                { PERIOD, "." },
                { F1, "F1" }, { F2, "F2" }, { F3, "F3" }, { F4, "F4" }, { F5, "F5" }, { F6, "F6" }, { F7, "F7" }, { F8, "F8" }, { F9, "F9" }, { F10, "F10" }, { F11, "F11" }, { F12, "F12" },
                { KEY_DELETE, "Delete" },
                { SPACE, "Space" }
            };

            bool changed = false;
            ShortcutBinding binding = EditorShortcuts::GetBinding(action);

            ImGui::PushID((int)action);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if(hasConflict)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
            }
            ImGui::TextUnformatted(EditorShortcuts::GetActionName(action));
            if(hasConflict)
            {
                ImGui::PopStyleColor();
            }
            ImGui::TableSetColumnIndex(1);

            changed |= ImGui::Checkbox("Ctrl", &binding.Ctrl);
            ImGui::SameLine();
            changed |= ImGui::Checkbox("Shift", &binding.Shift);
            ImGui::SameLine();
            changed |= ImGui::Checkbox("Alt", &binding.Alt);
            ImGui::SameLine();

            int selected = 0;
            for (int i = 0; i < IM_ARRAYSIZE(keyOptions); ++i)
            {
                if (keyOptions[i].Key == binding.Key)
                {
                    selected = i;
                    break;
                }
            }

            ImGui::SetNextItemWidth(120.f);
            if(ImGui::Combo("Key", &selected, [](void* data, int idx, const char** out_text)
            {
                KeyOption* options = (KeyOption*)data;
                *out_text = options[idx].Label;
                return true;
            }, (void*)keyOptions, IM_ARRAYSIZE(keyOptions)))
            {
                binding.Key = keyOptions[selected].Key;
                changed = true;
            }

            if(hasConflict)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Conflict");
            }

            if(changed)
            {
                EditorShortcuts::SetBinding(action, binding);
            }

            ImGui::PopID();
            return changed;
        }
        
    public:
        MenuBarWidget() {}

        static float GetSecondaryBarHeight()
        {
            return SecondaryBarHeight;
        }

        void Initialize(InputManager* inputManager) override
        {
            // inputManager->SubscribeToDynamicShortcut([]
            // {
            //     return EditorShortcuts::GetShortcut(EditorShortcutAction::ReloadScripts);
            // }, []
            // {
            //     ScriptEngine::ReloadScripts(true);
            // });
            //
            // inputManager->SubscribeToDynamicShortcut([]
            // {
            //     return EditorShortcuts::GetShortcut(EditorShortcutAction::ReloadShaders);
            // }, []
            // {
            //     Renderer::ReloadShaders();
            // });
            //
            // inputManager->SubscribeToDynamicShortcut([]
            // {
            //     return EditorShortcuts::GetShortcut(EditorShortcutAction::SaveScene);
            // }, [this]
            // {
            //     SaveScene();
            // });
            
            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::ReloadScripts, []
            {
                ScriptEngine::ReloadScripts(true);
            });
            
            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::ReloadShaders, []
            {
                Renderer::ReloadShaders();
            });
            
            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::SaveScene, [this]
            {
                SaveScene();
            });
        }

        void ExportScene(Path& path)
        {
            SceneManager::GetCurrentScene()->Serialize(path);
        }

        void SaveScene()
        {
            Path scenePath = SceneManager::GetCurrentScenePath();
            bool save = true;
            if(scenePath.empty())
            {
                save = SaveFile(scenePath, ".scene", L"Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0");
            }
                    
            if(save)
            {
                ExportScene(scenePath);
                SceneManager::SetCurrentScenePath(scenePath);
                WD_CORE_INFO("Scene was successfully saved!");
            }
        }

        void BuildProject(ProjectBuildConfiguration configuration)
        {
            Path outputDirectory = LastBuildDirectory;
            if(outputDirectory.empty())
            {
                outputDirectory = ProjectManager::HasProject()
                    ? ProjectManager::CurrentProject.ProjectPath.parent_path()
                    : GetCurrentFolder();
            }

            if(!SelectFolder(outputDirectory))
            {
                return;
            }

            LastBuildDirectory = outputDirectory;
            ProjectBuilder::BuildCurrentProject(outputDirectory, configuration);
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::BeginMainMenuBar())
            {
                float sceneLoadProgress = 0.0f;
                std::string sceneLoadLabel;
                const bool sceneLoading = SceneManager::GetLoadStatus(sceneLoadProgress, sceneLoadLabel);

                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open project", "Ctrl+O"))
                    {
                        Path selectedScenePath;
                        if(OpenFile(selectedScenePath, L"Project Files (*.wproject)\0*.wproject\0All Files (*.*)\0*.*\0"))
                        {
                            if(ProjectManager::LoadProject(selectedScenePath))
                            {
                                ScriptEngine::ReloadScripts(true);
                                Path startupScenePath = ProjectManager::CurrentProject.GetStartupScenePath();
                                SceneManager::LoadScene(startupScenePath);
                            }
                        }
                    }
                    
                    bool showCreateProject = CreateProjectWidget::IsVisible();
                    if (ImGui::MenuItem("Create project", nullptr, &showCreateProject))
                    {
                        CreateProjectWidget::SetVisible(showCreateProject);
                    }
                    // if (ImGui::MenuItem("New scene"))
                    // {
                    //     EditorCommandHistory::Get().Clear();
                    //     SceneManager::NewScene();
                    // }
                    // if (ImGui::MenuItem("Open scene", "Ctrl+O"))
                    // {
                    //     Path selectedScenePath;
                    //     if(OpenFile(selectedScenePath, L"Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0"))
                    //     {
                    //         EditorCommandHistory::Get().Clear();
                    //         SceneManager::LoadScene(selectedScenePath);
                    //     }
                    // } 
                    
                    if (ImGui::MenuItem("Save scene", "Ctrl+S"))
                    {
                        SaveScene();
                    }
                    
                    if (ImGui::MenuItem("Save scene as..."))
                    {
                        Path scenePath = SceneManager::GetCurrentScenePath();
                        if(SaveFile(scenePath, ".scene", L"Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0"))
                        {
                            ExportScene(scenePath);
                            SceneManager::SetCurrentScenePath(scenePath);
                        }
                    }

                    if (ImGui::MenuItem("Build project"))
                    {
                        BuildProject(ProjectBuildConfiguration::Release);
                    }
            
                    ImGui::Separator();
					       
                    if (ImGui::BeginMenu("Options"))
                    {
                        if (ImGui::BeginMenu("Shortcuts"))
                        {
                            bool changedAny = false;
                            std::unordered_map<uint64_t, int> bindingCounts;
                            for (int i = 0; i < EditorShortcuts::ActionCount(); ++i)
                            {
                                auto action = EditorShortcuts::ActionByIndex(i);
                                auto binding = EditorShortcuts::GetBinding(action);
                                bindingCounts[GetBindingKey(binding)]++;
                            }

                            int conflictsCount = 0;
                            for (const auto& pair : bindingCounts)
                            {
                                if(pair.second > 1)
                                {
                                    conflictsCount += pair.second;
                                }
                            }

                            if (ImGui::BeginTable("ShortcutTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
                            {
                                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.45f);
                                ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthStretch, 0.55f);

                                for (int i = 0; i < EditorShortcuts::ActionCount(); ++i)
                                {
                                    auto action = EditorShortcuts::ActionByIndex(i);
                                    auto binding = EditorShortcuts::GetBinding(action);
                                    bool hasConflict = bindingCounts[GetBindingKey(binding)] > 1;
                                    changedAny |= DrawShortcutRow(action, hasConflict);
                                }

                                ImGui::EndTable();
                            }

                            if(conflictsCount > 0)
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Conflicting shortcuts detected. Conflicting actions: %d", conflictsCount);
                            }

                            if (ImGui::MenuItem("Reset Defaults"))
                            {
                                EditorShortcuts::ResetDefaults();
                                changedAny = true;
                            }

                            if(changedAny)
                            {
                                EditorShortcuts::Save();
                            }

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenu();
                    }
					       
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Hot Reloading"))
                {
                    if (ImGui::MenuItem("Scripts"))
                    {
                        ScriptEngine::ReloadScripts(true);
                    }

                    if (ImGui::MenuItem("Shaders"))
                    {
                        Renderer::ReloadShaders();
                    }

                    ImGui::Separator();
                    ImGui::TextWrapped("Scripts: %s", ScriptEngine::GetLastReloadStatus());
                    ImGui::TextWrapped("Shaders: %s", Renderer::GetLastShaderReloadStatus());

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Window"))
                {
                    bool showCoach = CoachWidget::IsVisible();
                    if(ImGui::MenuItem("Coach", nullptr, &showCoach))
                    {
                        CoachWidget::SetVisible(showCoach);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Physics"))
                {
                    Vector3 gravity = PhysXSystem::GetGravity();
                    float gravityValues[3] = { gravity.x, gravity.y, gravity.z };
                    if (ImGui::DragFloat3("World Gravity", gravityValues, 0.1f))
                    {
                        PhysXSystem::SetGravity(Vector3(gravityValues[0], gravityValues[1], gravityValues[2]));
                    }
                    ImGui::EndMenu();
                }

                if (sceneLoading)
                {
                    ImGui::Separator();
                    ImGui::Text("Loading scene: %s", sceneLoadLabel.c_str());
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(180.0f);
                    ImGui::ProgressBar(sceneLoadProgress, ImVec2(180.0f, 0.0f));
                }
                ImGui::EndMainMenuBar();
            }

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            const float frameHeight = ImGui::GetFrameHeight();
            const float verticalPadding = ImGui::GetStyle().FramePadding.y * 2.0f;
            const float toolbarHeight = frameHeight + verticalPadding;
            SecondaryBarHeight = toolbarHeight;

            ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
            ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));
            ImGui::SetNextWindowViewport(viewport->ID);

            ImGuiWindowFlags toolbarFlags =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            if(ImGui::Begin("SimulationToolbar", nullptr, toolbarFlags))
            {
                const char* pauseLabel = EditorSimulation::IsPaused() ? "Resume" : "Pause";
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const bool showPlay = !EditorSimulation::IsPlaying();
                const bool showPause = EditorSimulation::IsPlaying();
                const bool showStop = EditorSimulation::IsPlaying();
                const float playWidth = showPlay ? (ImGui::CalcTextSize("Play").x + ImGui::GetStyle().FramePadding.x * 2.0f) : 0.0f;
                const float pauseWidth = showPause ? (ImGui::CalcTextSize(pauseLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f) : 0.0f;
                const float stopWidth = showStop ? (ImGui::CalcTextSize("Stop").x + ImGui::GetStyle().FramePadding.x * 2.0f) : 0.0f;
                const std::string modeLabel = std::string("Mode: ") + EditorSimulation::GetStateName();
                const float modeWidth = ImGui::CalcTextSize(modeLabel.c_str()).x;
                int visibleItems = (showPlay ? 1 : 0) + (showPause ? 1 : 0) + (showStop ? 1 : 0) + 1;
                const float totalWidth = playWidth + pauseWidth + stopWidth + modeWidth + spacing * (float)std::max(0, visibleItems - 1);
                bool needsSameLine = false;

                const float centeredX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
                if(centeredX > 0.0f)
                {
                    ImGui::SetCursorPosX(centeredX);
                }

                if(showPlay)
                {
                    if(ImGui::Button("Play"))
                    {
                        EditorSimulation::Play();
                    }
                    needsSameLine = true;
                }

                if(showPause)
                {
                    if(needsSameLine)
                    {
                        ImGui::SameLine();
                    }

                    if(ImGui::Button(pauseLabel))
                    {
                        if(EditorSimulation::IsPaused())
                        {
                            EditorSimulation::Play();
                        }
                        else
                        {
                            EditorSimulation::Pause();
                        }
                    }
                    needsSameLine = true;
                }

                if(showStop)
                {
                    if(needsSameLine)
                    {
                        ImGui::SameLine();
                    }

                    if(ImGui::Button("Stop"))
                    {
                        EditorSimulation::Stop();
                    }
                    needsSameLine = true;
                }

                if(needsSameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextUnformatted(modeLabel.c_str());
            }

            ImGui::End();
            ImGui::PopStyleVar(2);
        }
    };
}
