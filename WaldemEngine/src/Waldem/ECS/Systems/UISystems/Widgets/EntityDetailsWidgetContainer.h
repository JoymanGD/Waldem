#pragma once
#include <typeindex>

#include "imgui_internal.h"
#include "ComponentWidgets/ComponentWidgetSystem.h"
#include "Waldem/ECS/Components/AudioSource.h"
#include "Waldem/ECS/Components/BloomPostProcess.h"
#include "Waldem/ECS/Components/LineComponent.h"
#include "Waldem/ECS/Components/Ocean.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/ScriptComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/UISystems/Widgets/IWidgetContainerSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API EntityDetailsWidgetContainer : public IWidgetContainerSystem
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        
    public:
        EntityDetailsWidgetContainer(ECSManager* eCSManager, WArray<IWidgetSystem*> children) : IWidgetContainerSystem(eCSManager, children) {}

        WString GetName() override { return "Details"; }

        void Update(float deltaTime) override
        {
            if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                for (auto [entity, selected] : Manager->EntitiesWith<Selected>())
                {
                    for(auto child : Children)
                    {
                        IComponentWidgetSystem* componentWidget = dynamic_cast<IComponentWidgetSystem*>(child);
                        
                        if(componentWidget->IsVisible())
                        {
                            if (ImGui::BeginChild(componentWidget->GetName().C_Str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY))
                            {
                                if(componentWidget->IsRemovable() || componentWidget->IsResettable())
                                {
                                    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                                    if (ImGui::Button("..."))
                                    {
                                        ImGui::OpenPopup("TransformContextMenu");
                                    }

                                    if (ImGui::BeginPopupContextItem("TransformContextMenu", ImGuiPopupFlags_MouseButtonLeft))
                                    {
                                        if(componentWidget->IsResettable())
                                        {
                                            if (ImGui::MenuItem("Reset"))
                                            {
                                                componentWidget->ResetComponent(entity);
                                            }
                                        }

                                        if(componentWidget->IsRemovable())
                                        {
                                            if (ImGui::MenuItem("Remove"))
                                            {
                                                componentWidget->RemoveComponent(entity);
                                            }
                                        }

                                        ImGui::EndPopup();
                                    }
                                }

                                ImGui::Text(componentWidget->GetName().C_Str());
                                
                                componentWidget->Update(deltaTime);
                            }
                            ImGui::EndChild();
                            
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();
                        }
                    }

                    if (ImGui::Button("Add component"))
                    {
                        ImGui::OpenPopup("AddComponentPopup");
                    }

                    if (ImGui::BeginPopup("AddComponentPopup"))
                    {
                        static char searchBuffer[128] = "";
                        ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));

                        // Example list of components
                        static int selectedComponent = -1;

                        auto& componentNames = ComponentRegistry::Get().ComponentNames;

                        for (int i = 0; i < componentNames.Num(); i++)
                        {
                            if (strstr(componentNames[i], searchBuffer) != nullptr) // Filter by search
                            {
                                if (ImGui::Selectable(componentNames[i], selectedComponent == i))
                                {
                                    selectedComponent = i;

                                    //TODO: Add the selected component to the entity wrapper
                                    IComponentBase* comp = ComponentRegistry::Get().CreateComponent(componentNames[i]);
                                    comp->RegisterToNativeEntity(entity);
                                }
                            }
                        }

                        // if (ImGui::Button("Add Selected Component") && selectedComponent != -1)
                        // {
                        //     // Add the selected component to the entity
                        //     // Example: Manager->AddComponent(entity, components[selectedComponent]);
                        //     selectedComponent = -1; // Reset selection
                        //     ImGui::CloseCurrentPopup();
                        // }

                        ImGui::EndPopup();
                    }

                    break;
                }
            }
            ImGui::End();
            
        }
    };
}
