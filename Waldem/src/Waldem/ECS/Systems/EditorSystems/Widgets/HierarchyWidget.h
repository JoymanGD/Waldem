#pragma once
#include "Waldem/ECS/Systems/EditorSystems/WidgetSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API HierarchyWidget : public IWidgetSystem
    {
    private:

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
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            uint SelectedEntityId = -1;
            
            for (auto [selectedEntity, selected] : ECSManager->EntitiesWith<Selected>())
            {
                SelectedEntityId = selectedEntity.GetId();
                break;
            }
            
            ImGui::Begin("Entities");
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
            ImGui::End();
        }
    };
}
