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
        std::string RenameString = "";
        bool DeleteSelectedEntity = false;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

        void SelectEntity(ecs::Entity& entity)
        {
            DeselectEntity();

            entity.Add<Selected>();
        }

        void DeselectEntity()
        {
            for (auto [selectedEntity, selected] : Manager->EntitiesWith<Selected>())
            {
                selectedEntity.Remove<Selected>();
            }
        }
        
    public:
        HierarchyWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        WString GetName() override { return "Hierarchy"; }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            inputManager->SubscribeToKeyEvent(KEY_DELETE, [&](bool isPressed)
            {
                if(isPressed)
                {
                    DeleteSelectedEntity = true;
                }
            });
        }

        void Update(float deltaTime) override
        {
            int SelectedEntityId = -1;
            
            for (auto [selectedEntity, selected] : Manager->EntitiesWith<Selected>())
            {
                SelectedEntityId = selectedEntity.GetId();
                break;
            }

            if (ImGui::Begin("Entities", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("+").x - ImGui::GetStyle().FramePadding.x * 2);

                if (ImGui::Button("+"))
                {
                    auto entity = Manager->CreateEntity("NewEntity");
                    Manager->Refresh();
                    
                    SelectEntity(entity->NativeEntity);
                }

                ImGui::Separator();
                
                auto entities = Manager->Entities();
                // Iterate through entities and create selectable list
                for (auto [entity, nameComponent] : Manager->EntitiesWith<NameComponent>())
                {
                    if(entity.Has<EditorCamera>()) //Hide editor camera, we dont need to see it in the hierarchy
                        continue;
                    
                    int index = entity.GetId();
                    
                    bool isSelected = SelectedEntityId == index;
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
                            entity.Destroy();
                            DeselectEntity();
                            Manager->Refresh();
                        }
                    }
                }
            }
            ImGui::End();
                        
            DeleteSelectedEntity = false;
        }
    };
}