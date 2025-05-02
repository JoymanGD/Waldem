#pragma once
#include "imgui_internal.h"
#include "ComponentWidgets/ComponentWidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/IWidgetContainerSystem.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API EntityDetailsWidgetContainer : public IWidgetContainerSystem
    {
    private:
        float PanelWidth = 300.0f;
        float MinPanelWidth = 300.0f;
        float MaxPanelWidth = 500.0f;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings;
        
    public:
        EntityDetailsWidgetContainer(ECSManager* eCSManager, WArray<IWidgetSystem*> children) : IWidgetContainerSystem(eCSManager, children) {}

        WString GetName() override { return "Details"; }

        void Update(float deltaTime) override
        {
            // Stick to the left side and stretch vertically
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - PanelWidth, ImGui::GetFrameHeight()), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(PanelWidth, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
            ImGui::SetNextWindowSizeConstraints(ImVec2(MinPanelWidth, -1), ImVec2(MaxPanelWidth, -1));
            
            if (ImGui::Begin("Details", nullptr, WindowFlags))
            {
                PanelWidth = ImGui::GetWindowWidth();

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
                        // Add logic to handle adding a component
                    }

                    break;
                }
            }
            ImGui::End();
            
        }
    };
}
