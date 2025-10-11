#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem
{
    class GameViewportWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        
    public:
        GameViewportWidget() {}

        void OnDraw(float deltaTime) override
        {
            auto viewport = ViewportManager::GetGameViewport();
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (ImGui::Begin(viewport->Name, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {                
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
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}
