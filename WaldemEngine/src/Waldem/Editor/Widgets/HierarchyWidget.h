#pragma once
#include "Waldem/ECS/Components/SceneEntity.h"
#include "Widget.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Extensions/ImGUIExtension.h"

namespace Waldem
{
    struct WALDEM_API HierarchyEntry
    {
        WString Name;
        uint64 EntityId;
        uint64 ParentId;
        bool Enabled;
        bool Selected;
        bool VisibleInHierarchy;
    };
    
    class WALDEM_API HierarchyWidget : public IWidget
    {
        std::string RenameString = "";
        bool DeleteSelectedEntity = false;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        WMap<float, flecs::entity> HierarchyEntries;
        Shortcut DuplicateShortcut = { { LCTRL, D } };

        void SelectEntity(flecs::entity& entity)
        {
            DeselectAllEntities();

            entity.add<Selected>();
        }

        void DeselectAllEntities()
        {
            ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
            {
                selectedEntity.remove<Selected>();
            });
        }
        
    public:
        HierarchyWidget() {}

        WString GetName() override { return "Hierarchy"; }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
            inputManager->SubscribeToKeyEvent(KEY_DELETE, [&](bool isPressed)
            {
                if(isPressed)
                {
                    DeleteSelectedEntity = true;
                }
            });

            inputManager->SubscribeToShortcut(DuplicateShortcut, [&]
            {
                WArray<flecs::entity> selectedEntities;
                ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
                {
                    selectedEntities.Add(selectedEntity);
                });
                
                for (auto selectedEntity : selectedEntities)
                {
                    selectedEntity.remove<Selected>();
                    auto cloneEntity = ECS::CloneSceneEntity(selectedEntity);
                    cloneEntity.add<Selected>();
                }
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
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("+").x - ImGui::GetStyle().FramePadding.x * 2);

                if (ImGui::Button("+"))
                {
                    auto entity = ECS::CreateSceneEntity("NewEntity");
                        
                    SelectEntity(entity);
                }

                ImGui::Separator();
                
                for (int i = 0; i < HierarchyEntries.Num(); ++i)
                {
                    auto& entity = HierarchyEntries[i].value;
                    auto sceneEntityComponent = entity.get<SceneEntity>();
                
                    if(sceneEntityComponent->VisibleInHierarchy)
                    {
                        auto originalName = std::string(entity.name());
                        RenameString = originalName;

                        bool isSelected = entity.has<Selected>();

                        std::string id = "##Entity_" + std::to_string(i);
                        if (ImGui::SelectableInput(id, RenameString, isSelected, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                        {
                            if(RenameString.empty())
                            {
                                RenameString = "UnnamedEntity_" + std::to_string(i);
                            }

                            if(RenameString != originalName)
                            {
                                ECS::FormatName(RenameString);
                                entity.set_name(RenameString.c_str());
                            }

                            if(!isSelected)
                            {
                                SelectEntity(entity);
                            }
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();

                            if(DeleteSelectedEntity)
                            {
                                ECS::HierarchySlots.Free(sceneEntityComponent->HierarchySlot);
                                entity.destruct();
                            }
                        }
                    }
                }
            }
            ImGui::End();
                        
            DeleteSelectedEntity = false;
        }
    };
}