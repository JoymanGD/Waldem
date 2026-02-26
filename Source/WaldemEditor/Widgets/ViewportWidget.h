#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/Renderer/Viewport/Viewport.h"

namespace Waldem
{
    class ViewportWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        SViewport* Viewport = nullptr;
        
    public:
        ViewportWidget(SViewport* viewport) : Viewport(viewport) {}

        void OnDraw(float deltaTime) override
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (ImGui::Begin(Viewport->Name, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                ImVec2 viewportPos = ImGui::GetCursorScreenPos();
                ImVec2 viewportSize = ImGui::GetContentRegionAvail();

                if(viewportSize.x != Viewport->Size.x || viewportSize.y != Viewport->Size.y)
                {
                    Viewport->RequestResize(Point2(viewportSize.x, viewportSize.y));
                }

                if(viewportPos.x != Viewport->Position.x || viewportPos.y != Viewport->Position.y)
                {
                    Viewport->Move(Point2(viewportPos.x, viewportPos.y));
                }
                
                Viewport->IsMouseOver = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                
                auto renderTarget = Viewport->FrameBuffer->GetCurrentRenderTarget();
                ImGui::Image(renderTarget->GetGPUAddress(), viewportSize);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}
