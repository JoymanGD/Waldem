#pragma once
#include "Waldem/ECS/Systems/EditorSystems/Widgets/WidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Components/NameComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "misc/cpp/imgui_stdlib.h"

namespace Waldem
{
    class WALDEM_API NameWidget : public IWidgetSystem
    {
    private:
        String RenameString = "";
    public:
        NameWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        String GetName() override { return "Name"; }
        bool IsVisible() override { return Manager->EntitiesWith<NameComponent, Selected>().Count() > 0; }

        void Update(float deltaTime) override
        {
            for (auto [entity, nameComponent, selected] : Manager->EntitiesWith<NameComponent, Selected>())
            {
                RenameString = nameComponent.Name;
                
                String id = "##Entity_" + std::to_string(entity.GetId());
                
                if(ImGui::InputText(id.c_str(), &RenameString, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    nameComponent.Name = RenameString;
                }
            }
        }
    };
}
