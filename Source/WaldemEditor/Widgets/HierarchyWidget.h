#pragma once
#include "imgui_internal.h"
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
#include "Waldem/SceneManagement/ModelSpawner.h"
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
        static constexpr const char* ModelDragPayloadType = "Model";
        std::string RenameString = "";
        bool DeleteSelectedEntity = false;
        bool RenameSelectedEntity = false;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        WMap<float, ECS::Entity> HierarchyEntries;
        std::unordered_map<flecs::entity_t, bool> ExpandedByEntity;
        flecs::entity_t SelectionAnchorEntity = 0;

        std::vector<flecs::entity_t> GetSelectedEntityIds() const
        {
            std::vector<flecs::entity_t> selectedIds;
            ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
            {
                selectedIds.push_back(selectedEntity.id());
            });

            return selectedIds;
        }

        void SelectEntity(flecs::entity& entity)
        {
            DeselectAllEntities();

            entity.add<Selected>();
            SelectionAnchorEntity = entity.id();
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
                constexpr float hierarchyFoldAreaWidth = 14.0f;
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

                auto instantiateModelFromPayload = [&](const ImGuiPayload* payload, ECS::Entity parentEntity = ECS::Entity{})
                {
                    const char* relativeModelPath = static_cast<const char*>(payload->Data);
                    if(relativeModelPath == nullptr || relativeModelPath[0] == '\0')
                    {
                        return;
                    }

                    Path modelPath = Path(CONTENT_PATH) / Path(relativeModelPath);
                    if(modelPath.extension() != ".model")
                    {
                        return;
                    }

                    auto spawnedRoot = ModelSpawner::InstantiateModel(modelPath, parentEntity);
                    if(spawnedRoot.is_alive())
                    {
                        SelectEntity(spawnedRoot);
                    }
                };

                std::function<void(flecs::entity_t, int)> drawEntityRecursive;
                std::vector<flecs::entity_t> visibleHierarchyOrder;
                std::function<void(flecs::entity_t)> buildVisibleOrder;
                buildVisibleOrder = [&](flecs::entity_t entityId)
                {
                    visibleHierarchyOrder.push_back(entityId);

                    auto childrenIt = childrenByParent.find(entityId);
                    const bool hasChildren = childrenIt != childrenByParent.end() && !childrenIt->second.empty();
                    if(!hasChildren)
                    {
                        return;
                    }

                    auto expandedIt = ExpandedByEntity.find(entityId);
                    const bool isExpanded = expandedIt == ExpandedByEntity.end() ? true : expandedIt->second;
                    if(!isExpanded)
                    {
                        return;
                    }

                    for (auto childId : childrenIt->second)
                    {
                        buildVisibleOrder(childId);
                    }
                };

                for (auto rootId : roots)
                {
                    buildVisibleOrder(rootId);
                }

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
                    auto childrenIt = childrenByParent.find(entityId);
                    const bool hasChildren = childrenIt != childrenByParent.end() && !childrenIt->second.empty();

                    const float rowStartX = hierarchyBaseX + (float)depth * hierarchyIndentWidth;
                    ImGui::SetCursorPosX(rowStartX + hierarchyFoldAreaWidth + 4.0f);

                    if (hasChildren)
                    {
                        auto expandedIt = ExpandedByEntity.find(entityId);
                        if (expandedIt == ExpandedByEntity.end())
                        {
                            expandedIt = ExpandedByEntity.emplace(entityId, true).first;
                        }
                        bool& isExpanded = expandedIt->second;

                        const ImVec2 rowStartScreenPos = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos(ImVec2(rowStartScreenPos.x - hierarchyFoldAreaWidth - 4.0f, rowStartScreenPos.y));
                        ImGui::PushID((int)entityId);
                        const float rowTextHeight = ImGui::GetTextLineHeight();
                        if (ImGui::InvisibleButton("##ExpandCollapse", ImVec2(hierarchyFoldAreaWidth, rowTextHeight)))
                        {
                            isExpanded = !isExpanded;
                        }
                        const ImVec2 foldMin = ImGui::GetItemRectMin();
                        const float arrowScale = 0.70f;
                        const float arrowSize = ImGui::GetFontSize() * arrowScale;
                        const ImVec2 arrowPos(
                            foldMin.x + (hierarchyFoldAreaWidth - arrowSize) * 0.5f,
                            rowStartScreenPos.y + (rowTextHeight - arrowSize) * 0.5f
                        );
                        ImGui::RenderArrow(
                            ImGui::GetWindowDrawList(),
                            arrowPos,
                            ImGui::GetColorU32(ImGuiCol_Text),
                            isExpanded ? ImGuiDir_Down : ImGuiDir_Right,
                            arrowScale);
                        ImGui::PopID();
                        ImGui::SetCursorScreenPos(rowStartScreenPos);
                    }

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

                    }

                    if(startRename)
                    {
                        RenameSelectedEntity = false;
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        const bool ctrlHeld = ImGui::GetIO().KeyCtrl;
                        const bool shiftHeld = ImGui::GetIO().KeyShift;

                        if (shiftHeld && SelectionAnchorEntity != 0)
                        {
                            auto anchorIt = std::find(visibleHierarchyOrder.begin(), visibleHierarchyOrder.end(), SelectionAnchorEntity);
                            auto currentIt = std::find(visibleHierarchyOrder.begin(), visibleHierarchyOrder.end(), entityId);

                            if(anchorIt != visibleHierarchyOrder.end() && currentIt != visibleHierarchyOrder.end())
                            {
                                if(!ctrlHeld)
                                {
                                    DeselectAllEntities();
                                }

                                auto rangeBegin = anchorIt < currentIt ? anchorIt : currentIt;
                                auto rangeEnd = anchorIt < currentIt ? currentIt : anchorIt;

                                for (auto it = rangeBegin; it <= rangeEnd; ++it)
                                {
                                    auto rangeEntity = ECS::World.entity(*it);
                                    if(rangeEntity.is_alive())
                                    {
                                        rangeEntity.add<Selected>();
                                    }
                                }

                                SelectionAnchorEntity = entity.id();
                            }
                            else
                            {
                                SelectEntity(entity);
                            }
                        }
                        else if (ctrlHeld)
                        {
                            if (entity.has<Selected>())
                            {
                                entity.remove<Selected>();
                            }
                            else
                            {
                                entity.add<Selected>();
                            }

                            SelectionAnchorEntity = entity.id();
                        }
                        else
                        {
                            if(!entity.has<Selected>())
                            {
                                SelectEntity(entity);
                            }
                            else
                            {
                                SelectionAnchorEntity = entity.id();
                            }
                        }
                    }

                    if (ImGui::BeginDragDropSource())
                    {
                        const auto draggedEntityId = entity.id();
                        ImGui::SetDragDropPayload(HierarchyDragPayloadType, &draggedEntityId, sizeof(draggedEntityId));

                        std::vector<flecs::entity_t> draggedSelectionIds;
                        if(entity.has<Selected>())
                        {
                            draggedSelectionIds = GetSelectedEntityIds();
                        }
                        else
                        {
                            draggedSelectionIds.push_back(draggedEntityId);
                        }

                        if(draggedSelectionIds.size() > 1)
                        {
                            ImGui::Text("%zu entities", draggedSelectionIds.size());
                        }
                        else
                        {
                            ImGui::TextUnformatted(entity.name().c_str());
                        }
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
                                std::vector<flecs::entity_t> entitiesToReparent;
                                if(draggedEntity.has<Selected>())
                                {
                                    entitiesToReparent = GetSelectedEntityIds();
                                }
                                else
                                {
                                    entitiesToReparent.push_back(draggedEntityId);
                                }

                                for (auto reparentEntityId : entitiesToReparent)
                                {
                                    if(reparentEntityId == entity.id())
                                    {
                                        continue;
                                    }

                                    auto reparentEntity = ECS::World.entity(reparentEntityId);
                                    if(reparentEntity.is_alive())
                                    {
                                        ECS::SetParent(reparentEntity, entity, true);
                                    }
                                }
                            }
                        }
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PrefabDragPayloadType))
                        {
                            instantiatePrefabFromPayload(payload, entity);
                        }
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ModelDragPayloadType))
                        {
                            instantiateModelFromPayload(payload, entity);
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if(hasChildren && ExpandedByEntity[entityId])
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
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    DeselectAllEntities();
                    SelectionAnchorEntity = 0;
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HierarchyDragPayloadType))
                    {
                        auto draggedEntityId = *(const flecs::entity_t*)payload->Data;
                        auto draggedEntity = ECS::World.entity(draggedEntityId);

                        if (draggedEntity.is_alive())
                        {
                            std::vector<flecs::entity_t> entitiesToUnparent;
                            if(draggedEntity.has<Selected>())
                            {
                                entitiesToUnparent = GetSelectedEntityIds();
                            }
                            else
                            {
                                entitiesToUnparent.push_back(draggedEntityId);
                            }

                            for (auto unparentEntityId : entitiesToUnparent)
                            {
                                auto unparentEntity = ECS::World.entity(unparentEntityId);
                                if(unparentEntity.is_alive())
                                {
                                    ECS::ClearParent(unparentEntity, true);
                                }
                            }
                        }
                    }
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PrefabDragPayloadType))
                    {
                        instantiatePrefabFromPayload(payload);
                    }
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ModelDragPayloadType))
                    {
                        instantiateModelFromPayload(payload);
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
