#pragma once
#include "imgui_internal.h"
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
                
                if(!Manager->EntitiesWith<Selected>().GetVector().empty())
                {
                    for(auto child : Children)
                    {
                        if(child->IsVisible())
                        {
                            if (ImGui::BeginChild(child->GetName().C_Str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY))
                            {
                                if(child->IsRemovable() || child->IsResettable())
                                {
                                    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                                    if (ImGui::Button("..."))
                                    {
                                        ImGui::OpenPopup("TransformContextMenu");
                                    }

                                    if (ImGui::BeginPopupContextItem("TransformContextMenu", ImGuiPopupFlags_MouseButtonLeft))
                                    {
                                        if(child->IsResettable())
                                        {
                                            if (ImGui::MenuItem("Reset"))
                                            {
                                            }
                                        }

                                        if(child->IsRemovable())
                                        {
                                            if (ImGui::MenuItem("Remove"))
                                            {
                                            }
                                        }

                                        ImGui::EndPopup();
                                    }
                                }

                                ImGui::Text(child->GetName().C_Str());
                                
                                child->Update(deltaTime);
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
                }
            }
            ImGui::End();
            
        }
    };
}
