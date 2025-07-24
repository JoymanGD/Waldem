#pragma once
#include "Waldem/ECS/Components/NameComponent.h"
#include "Waldem/ECS/Systems/UISystems/Widgets/WidgetSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Extensions/ImGUIExtension.h"

namespace Waldem
{
    class WALDEM_API HierarchyWidget : public IWidgetSystem
    {
        std::string RenameString = "";
        bool DeleteSelectedEntity = false;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        WArray<size_t> SelectedEntityIds = {};
        flecs::query<NameComponent> NameQuery;

        void SelectEntity(flecs::entity& entity)
        {
            DeselectAllEntity();

            entity.add<Selected>();
        }

        void DeselectAllEntity()
        {
            ECS::World.query<Selected>().each([&](flecs::entity selectedEntity, Selected)
            {
                selectedEntity.remove<Selected>();
            });
        }
        
    public:
        HierarchyWidget() {}

        WString GetName() override { return "Hierarchy"; }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            inputManager->SubscribeToKeyEvent(KEY_DELETE, [&](bool isPressed)
            {
                if(isPressed)
                {
                    DeleteSelectedEntity = true;
                }
            });

            ECS::World.observer<Selected>().event(flecs::OnAdd).each([&](flecs::entity entity, Selected)
            {
                SelectedEntityIds.Add(entity.id());
            });

            ECS::World.observer<Selected>().event(flecs::OnRemove).each([&](flecs::entity entity, Selected)
            {
                SelectedEntityIds.Remove(entity.id());
            });

            NameQuery = ECS::World.query_builder<NameComponent>()
                .without<EditorCamera>()
                .build();
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Entities", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("+").x - ImGui::GetStyle().FramePadding.x * 2);

                if (ImGui::Button("+"))
                {
                    auto entity = ECS::World.entity("NewEntity");
                    
                    SelectEntity(entity);
                }

                ImGui::Separator();
                
                NameQuery.each([&](flecs::entity entity, NameComponent& nameComponent)
                {
                    auto index = entity.id();
                    
                    bool isSelected = SelectedEntityIds.Contains(index);
                    RenameString = nameComponent.Name;

                    std::string id = "##Entity_" + std::to_string(index);
                    if (ImGui::SelectableInput(id, RenameString, isSelected, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        if(RenameString.empty())
                        {
                            RenameString = "UnnamedEntity_" + std::to_string(index);
                        }

                        nameComponent.Name = RenameString;
                        SelectEntity(entity);
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();

                        if(DeleteSelectedEntity)
                        {
                            entity.destruct();
                            DeselectAllEntity();
                        }
                    }
                });
            }
            ImGui::End();
                        
            DeleteSelectedEntity = false;
        }
    };
}