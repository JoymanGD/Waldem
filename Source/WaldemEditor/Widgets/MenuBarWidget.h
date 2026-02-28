#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Utils/FileUtils.h"
#include "Commands/EditorCommands.h"
#include "../EditorShortcuts.h"
#include <cstdint>
#include <unordered_map>
#include <string>

namespace Waldem
{
    class MenuBarWidget : public IWidget
    {
    private:
        struct KeyOption
        {
            int Key;
            const char* Label;
        };

        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        Path CurrentScenePath;

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
                float sceneLoadProgress = 0.0f;
                std::string sceneLoadLabel;
                const bool sceneLoading = SceneManager::GetLoadStatus(sceneLoadProgress, sceneLoadLabel);

                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New scene"))
                    {
                        EditorCommandHistory::Get().Clear();
                        SceneManager::NewScene();
                    }
                    if (ImGui::MenuItem("Open scene", "Ctrl+O"))
                    {
                        if(OpenFile(CurrentScenePath))
                        {
                            EditorCommandHistory::Get().Clear();
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
        }
    };
}
