#pragma once
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Components/NameComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "misc/cpp/imgui_stdlib.h"

namespace Waldem
{
    class WALDEM_API NameWidget : public ComponentWidget<NameComponent>
    {
    private:
        WString RenameString = "";
    public:
        NameWidget(ECSManager* eCSManager) : ComponentWidget(eCSManager) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override {}

        WString GetName() override { return "Name"; }
        bool IsRemovable() override { return false; }
        bool IsResettable() override { return false; }

        void Update(float deltaTime) override
        {
            for (auto [entity, nameComponent, selected] : Manager->EntitiesWith<NameComponent, Selected>())
            {
                RenameString = nameComponent.Name;
                
                WString id = "##Entity_" + std::to_string(entity.GetId());
                
                if(ImGui::InputText(id.C_Str(), RenameString.GetData(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    nameComponent.Name = RenameString;
                }
            }
        }
    };
}
