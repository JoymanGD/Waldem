#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem
{
    class GameViewportWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        int SelectedRenderTarget = 0;
        
    public:
        GameViewportWidget() {}

        void OnDraw(float deltaTime) override
        {
            auto viewport = ViewportManager::GetGameViewport();
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (ImGui::Begin(viewport->Name, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                const char* renderTargets[] =
                {
                    "Final",
                    "Normals",
                    "Reflections",
                    "WorldPos",
                    "ORM",
                    "MeshID",
                    "Color",
                    "Radiance"
                };

                ImVec2 viewportPos = ImGui::GetCursorScreenPos();
                ImVec2 viewportSize = ImGui::GetContentRegionAvail();

                if(viewportSize.x != viewport->Size.x || viewportSize.y != viewport->Size.y)
                {
                    viewport->RequestResize(Point2(viewportSize.x, viewportSize.y));
                }

                if(viewportPos.x != viewport->Position.x || viewportPos.y != viewport->Position.y)
                {
                    viewport->Move(Point2(viewportPos.x, viewportPos.y));
                }
                
                viewport->IsMouseOver = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                
                auto renderTarget = viewport->FrameBuffer->GetCurrentRenderTarget();
                ImGui::Image(renderTarget->GetGPUAddress(), viewportSize);

                const float buttonWidth = 26.0f;
                const float buttonHeight = 22.0f;
                const float buttonPadding = 8.0f;
                ImGui::SetCursorScreenPos(ImVec2(
                    viewportPos.x + viewportSize.x - buttonWidth - buttonPadding,
                    viewportPos.y + buttonPadding));

                if(ImGui::Button("...##GameViewportOptions", ImVec2(buttonWidth, buttonHeight)))
                {
                    ImGui::OpenPopup("GameViewportOptionsPopup");
                }

                if(ImGui::BeginPopup("GameViewportOptionsPopup"))
                {
                    ImGui::SetNextItemWidth(180.0f);
                    ImGui::Combo("Render Target", &SelectedRenderTarget, renderTargets, IM_ARRAYSIZE(renderTargets));
                    ImGui::Separator();

                    auto& toggles = Renderer::RenderData.FeatureToggles;
                    ImGui::Checkbox("Sky##Game", &toggles.EnableSkyPass);
                    ImGui::Checkbox("GBuffer##Game", &toggles.EnableGBufferPass);
                    ImGui::Checkbox("Ray Tracing##Game", &toggles.EnableRayTracingPass);
                    ImGui::Checkbox("Reflections##Game", &toggles.EnableReflections);
                    ImGui::Checkbox("Direct Lighting##Game", &toggles.EnableDirectLighting);
                    ImGui::Checkbox("Specular##Game", &toggles.EnableSpecular);
                    ImGui::Checkbox("Metallic##Game", &toggles.EnableMetallic);
                    ImGui::EndPopup();
                }

                Renderer::RenderData.GameViewportOutputTarget = SelectedRenderTarget;
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}
