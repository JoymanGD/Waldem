#pragma once
#include "imgui_internal.h"
#include "Waldem/ECS/Systems/EditorSystems/WidgetSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API HierarchyWidget : public IWidgetSystem
    {
    private:
        float PanelWidth = 300.0f;
        float MinPanelWidth = 300.0f;
        float MaxPanelWidth = 500.0f;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;

        void SelectEntity(ecs::Entity& entity)
        {
            for (auto [selectedEntity, selected] : ECSManager->EntitiesWith<Selected>())
            {
                selectedEntity.Remove<Selected>();
            }

            entity.Add<Selected>();
        }
        
    public:
        HierarchyWidget(ecs::Manager* eCSManager) : IWidgetSystem(eCSManager) {}

        String GetName() override { return "Hierarchy"; }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            uint SelectedEntityId = -1;
            
            for (auto [selectedEntity, selected] : ECSManager->EntitiesWith<Selected>())
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
            
                // Iterate through entities and create selectable list
                for (auto [entity, nameComponent] : ECSManager->EntitiesWith<NameComponent>())
                {
                    bool isSelected = SelectedEntityId == entity.GetId();

                    if (ImGui::Selectable(nameComponent.Name.c_str(), isSelected))
                    {
                        SelectEntity(entity);
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::End();
            
            // ImGui::Begin("Entities");
            // for (auto [entity, nameComponent] : ECSManager->EntitiesWith<NameComponent>())
            // {
            //     bool isSelected = SelectedEntityId == entity.GetId();
            //     
            //     if (ImGui::Selectable(nameComponent.Name.c_str(), isSelected))
            //     {
            //         SelectEntity(entity);
            //     }
            //     
            //     if (isSelected)
            //     {
            //         ImGui::SetItemDefaultFocus();
            //     }
            // }
            // ImGui::End();
        }
    };
}
