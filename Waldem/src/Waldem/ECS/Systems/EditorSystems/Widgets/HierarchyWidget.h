#pragma once
#include "Waldem/ECS/Components/NameComponent.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/WidgetSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Extensions/ImGUIExtension.h"

namespace Waldem
{
    class WALDEM_API HierarchyWidget : public IWidgetSystem
    {
    private:
        String RenameString = "";
        float PanelWidth = 300.0f;
        float MinPanelWidth = 300.0f;
        float MaxPanelWidth = 500.0f;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;

        void SelectEntity(ecs::Entity& entity)
        {
            for (auto [selectedEntity, selected] : Manager->EntitiesWith<Selected>())
            {
                selectedEntity.Remove<Selected>();
            }

            entity.Add<Selected>();
        }
        
    public:
        HierarchyWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        String GetName() override { return "Hierarchy"; }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            int SelectedEntityId = -1;
            
            for (auto [selectedEntity, selected] : Manager->EntitiesWith<Selected>())
            {
                SelectedEntityId = selectedEntity.GetId();
                break;
            }

            // Stick to the left side and stretch vertically
            ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(PanelWidth, ImGui::GetIO().DisplaySize.y), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(MinPanelWidth, -1), ImVec2(MaxPanelWidth, -1));

            if (ImGui::Begin("Entities", nullptr, WindowFlags))
            {
                PanelWidth = ImGui::GetWindowWidth();

                auto entities = Manager->Entities();
                // Iterate through entities and create selectable list
                for (auto [entity, nameComponent] : Manager->EntitiesWith<NameComponent>())
                {
                    if(entity.Has<EditorCamera>()) //Hide editor camera, we dont need to see it in the hierarchy
                        continue;
                    
                    int index = entity.GetId();
                    
                    bool isSelected = SelectedEntityId == index;
                    RenameString = nameComponent.Name;

                    String id = "##Entity_" + std::to_string(index);
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
                    }
                }
            }
            ImGui::End();
        }
    };
}