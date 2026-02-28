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
#include "Waldem/SceneManagement/Prefab.h"
#include "Commands/EditorCommands.h"
#include "../EditorShortcuts.h"
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Waldem
{
    class HierarchyWidget : public IWidget
    {
        static constexpr const char* HierarchyDragPayloadType = "WALDEM_HIERARCHY_ENTITY";
        static constexpr const char* PrefabDragPayloadType = "Prefab";
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

                std::unordered_map<flecs::entity_t, ECS::Entity> entitiesById;
                std::unordered_map<flecs::entity_t, flecs::entity_t> parentByEntity;
                std::unordered_map<flecs::entity_t, float> slotByEntity;
                std::unordered_map<flecs::entity_t, std::vector<flecs::entity_t>> childrenByParent;
                std::vector<flecs::entity_t> roots;

                for (int i = 0; i < HierarchyEntries.Num(); ++i)
                {
                    auto& entity = HierarchyEntries[i].value;
                    if(!entity.is_alive() || !entity.has<SceneEntity>())
                    {
                        continue;
                    }

                    auto& sceneEntityComponent = entity.get<SceneEntity>();
                    if(!sceneEntityComponent.VisibleInHierarchy)
                    {
                        continue;
                    }

                    entitiesById[entity.id()] = entity;
                    slotByEntity[entity.id()] = sceneEntityComponent.HierarchySlot;
                }

                for (const auto& [entityId, entity] : entitiesById)
                {
                    auto parent = ECS::GetParent(entity);
                    auto parentId = parent.is_alive() ? parent.id() : 0;
                    if(parentId != 0 && entitiesById.find(parentId) == entitiesById.end())
                    {
                        parentId = 0;
                    }

                    parentByEntity[entityId] = parentId;
                }

                for (const auto& [entityId, parentId] : parentByEntity)
                {
                    if(parentId == 0)
                    {
                        roots.push_back(entityId);
                    }
                    else
                    {
                        childrenByParent[parentId].push_back(entityId);
                    }
                }

                auto sortByHierarchySlot = [&](std::vector<flecs::entity_t>& ids)
                {
                    std::sort(ids.begin(), ids.end(), [&](flecs::entity_t lhs, flecs::entity_t rhs)
                    {
                        return slotByEntity[lhs] < slotByEntity[rhs];
                    });
                };

                sortByHierarchySlot(roots);
                for (auto& [_, children] : childrenByParent)
                {
                    sortByHierarchySlot(children);
                }

                int hierarchyDrawIndex = 0;
                const float hierarchyBaseX = ImGui::GetCursorPosX();
                constexpr float hierarchyIndentWidth = 20.0f;
                auto instantiatePrefabFromPayload = [&](const ImGuiPayload* payload, ECS::Entity parentEntity = ECS::Entity{})
                {
                    const char* relativePrefabPath = static_cast<const char*>(payload->Data);
                    if(relativePrefabPath == nullptr || relativePrefabPath[0] == '\0')
                    {
                        return;
                    }

                    Path prefabPath = Path(CONTENT_PATH) / Path(relativePrefabPath);
                    if(prefabPath.extension() != ".prefab")
                    {
                        return;
                    }

                    auto spawnedRoot = Prefab::InstantiatePrefab(prefabPath, parentEntity);
                    if(spawnedRoot.is_alive())
                    {
                        SelectEntity(spawnedRoot);
                    }
                };

                std::function<void(flecs::entity_t, int)> drawEntityRecursive;
                drawEntityRecursive = [&](flecs::entity_t entityId, int depth)
                {
                    auto entityIt = entitiesById.find(entityId);
                    if(entityIt == entitiesById.end())
                    {
                        return;
                    }

                    auto entity = entityIt->second;
                    auto originalName = std::string(entity.name());
                    RenameString = originalName;

                    bool isSelected = entity.has<Selected>();
                    bool startRename = RenameSelectedEntity && hierarchyFocused && isSelected;

                    ImGui::SetCursorPosX(hierarchyBaseX + (float)depth * hierarchyIndentWidth);

                    std::string id = "##Entity_" + std::to_string((uint64)entity.id()) + "_" + std::to_string(hierarchyDrawIndex++);
                    if (ImGui::SelectableInput(id, RenameString, isSelected, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll, startRename))
                    {
                        if(RenameString.empty())
                        {
                            RenameString = "UnnamedEntity_" + std::to_string(hierarchyDrawIndex);
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

                    if (ImGui::BeginDragDropSource())
                    {
                        const auto draggedEntityId = entity.id();
                        ImGui::SetDragDropPayload(HierarchyDragPayloadType, &draggedEntityId, sizeof(draggedEntityId));
                        ImGui::TextUnformatted(entity.name().c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                        {
                            auto draggedEntityId = *(const flecs::entity_t*)payload->Data;
                            auto draggedEntity = ECS::World.entity(draggedEntityId);

                            if (draggedEntity.is_alive() && draggedEntityId != entity.id())
                            {
                                ECS::SetParent(draggedEntity, entity, true);
                            }
                        }
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PrefabDragPayloadType))
                        {
                            instantiatePrefabFromPayload(payload, entity);
                        }
                        ImGui::EndDragDropTarget();
                    }

                    auto childrenIt = childrenByParent.find(entityId);
                    if(childrenIt != childrenByParent.end())
                    {
                        for (auto childId : childrenIt->second)
                        {
                            drawEntityRecursive(childId, depth + 1);
                        }
                    }
                };

                for (auto rootId : roots)
                {
                    drawEntityRecursive(rootId, 0);
                }

                float emptyDropHeight = ImGui::GetContentRegionAvail().y;
                if (emptyDropHeight < 24.0f)
                {
                    emptyDropHeight = 24.0f;
                }

                ImGui::InvisibleButton("##HierarchyEmptyDropTarget", ImVec2(ImGui::GetContentRegionAvail().x, emptyDropHeight));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                    {
                        auto draggedEntityId = *(const flecs::entity_t*)payload->Data;
                        auto draggedEntity = ECS::World.entity(draggedEntityId);

                        if (draggedEntity.is_alive())
                        {
                            ECS::ClearParent(draggedEntity, true);
                        }
                    }
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PrefabDragPayloadType))
                    {
                        instantiatePrefabFromPayload(payload);
                    }
                    ImGui::EndDragDropTarget();
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
