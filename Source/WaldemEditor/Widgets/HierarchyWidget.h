#pragma once
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/SceneEntity.h"
#include "Widget.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Extensions/ImGUIExtension.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Types/WMap.h"
#include "Waldem/Utils/ECSUtils.h"
#include "Commands/EditorCommands.h"
#include "../EditorShortcuts.h"

namespace Waldem
{
    class HierarchyWidget : public IWidget
    {
        std::string RenameString = "";
        bool DeleteSelectedEntity = false;
        bool RenameSelectedEntity = false;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        WMap<float, ECS::Entity> HierarchyEntries;

        void SelectEntity(flecs::entity& entity)
        {
            DeselectAllEntities();

            entity.add<Selected>();
        }

        void DeselectAllEntities()
        {
            ECS::World.defer_begin();
            ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
            {
                selectedEntity.remove<Selected>();
            });
            ECS::World.defer_end();
        }
        
    public:
        HierarchyWidget() {}

        void Initialize(InputManager* inputManager) override
        {
            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::DeleteEntity);
            }, [&]
            {
                DeleteSelectedEntity = true;
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::RenameEntity);
            }, [&]
            {
                RenameSelectedEntity = true;
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::DuplicateEntity);
            }, [&]
            {
                WArray<flecs::entity> selectedEntities;
                ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
                {
                    selectedEntities.Add(selectedEntity);
                });
                
                for (auto selectedEntity : selectedEntities)
                {
                    selectedEntity.remove<Selected>();
                    auto cloneCommand = std::make_unique<CloneSceneEntityCommand>(selectedEntity.id());
                    auto cloneCommandPtr = cloneCommand.get();
                    EditorCommandHistory::Get().Execute(std::move(cloneCommand));

                    auto cloneEntity = ECS::World.entity(cloneCommandPtr->GetCloneId());
                    cloneEntity.add<Selected>();
                }
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::Undo);
            }, [&]
            {
                EditorCommandHistory::Get().Undo();
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::Redo);
            }, [&]
            {
                EditorCommandHistory::Get().Redo();
            });
            
            ECS::World.observer<SceneEntity>("HierarchyWidgetSortSystemOnAdd").event(flecs::OnSet).each([&](flecs::entity entity, SceneEntity& sceneEntity)
            {
                if(!HierarchyEntries.Contains(sceneEntity.HierarchySlot))
                {
                    HierarchyEntries[sceneEntity.HierarchySlot] = entity;
                }
            });
            
            ECS::World.observer<SceneEntity>("HierarchyWidgetSortSystemOnRemove").event(flecs::OnRemove).each([&](flecs::entity entity, SceneEntity& sceneEntity)
            {
                HierarchyEntries.Remove(sceneEntity.HierarchySlot);
            });
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Entities", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                const bool hierarchyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

                if (ImGui::Button("+  New Entity", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                {
                    auto createCommand = std::make_unique<CreateSceneEntityCommand>("NewEntity");
                    auto createCommandPtr = createCommand.get();
                    EditorCommandHistory::Get().Execute(std::move(createCommand));

                    auto entity = ECS::World.entity(createCommandPtr->GetEntityId());
                        
                    SelectEntity(entity);
                }

                ImGui::Separator();
                
                for (int i = 0; i < HierarchyEntries.Num(); ++i)
                {
                    auto& entity = HierarchyEntries[i].value;
                    auto& sceneEntityComponent = entity.get<SceneEntity>();
                
                    if(sceneEntityComponent.VisibleInHierarchy)
                    {
                        auto originalName = std::string(entity.name());
                        RenameString = originalName;

                        bool isSelected = entity.has<Selected>();
                        bool startRename = RenameSelectedEntity && hierarchyFocused && isSelected;

                        std::string id = "##Entity_" + std::to_string(i);
                        if (ImGui::SelectableInput(id, RenameString, isSelected, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll, startRename))
                        {
                            if(RenameString.empty())
                            {
                                RenameString = "UnnamedEntity_" + std::to_string(i);
                            }

                            if(RenameString != originalName)
                            {
                                FormatName(RenameString);
                                EditorCommandHistory::Get().Execute(std::make_unique<RenameEntityCommand>(entity.id(), originalName.c_str(), RenameString.c_str()));
                            }

                            if(!isSelected)
                            {
                                SelectEntity(entity);
                            }
                        }

                        if(startRename)
                        {
                            RenameSelectedEntity = false;
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }
            }
            ImGui::End();

            if(DeleteSelectedEntity)
            {
                WArray<flecs::entity> selectedEntities;
                ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
                {
                    selectedEntities.Add(selectedEntity);
                });

                for (auto selectedEntity : selectedEntities)
                {
                    EditorCommandHistory::Get().Execute(std::make_unique<DeleteSceneEntityCommand>(selectedEntity));
                }
            }
            
            DeleteSelectedEntity = false;
            RenameSelectedEntity = false;
        }
    };
}
