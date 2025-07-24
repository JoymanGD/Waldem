#pragma once
#include "WidgetContainerSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Viewport.h"

namespace Waldem
{
    class WALDEM_API MainViewportWidget : public IWidgetSystem
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        
    public:
        MainViewportWidget() {}

        WString GetName() override { return "Viewport"; }

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                auto editorViewport = Renderer::GetEditorViewport();
                
                ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
                
                ImVec2 viewportPos = ImGui::GetCursorScreenPos();
                ImVec2 viewportSize = ImGui::GetContentRegionAvail();

                if(viewportSize.x != editorViewport->Size.x || viewportSize.y != editorViewport->Size.y)
                {
                    editorViewport->Resize(Point2(viewportSize.x, viewportSize.y));
                }

                if(viewportPos.x != editorViewport->Position.x || viewportPos.y != editorViewport->Position.y)
                {
                    editorViewport->Move(Point2(viewportPos.x, viewportPos.y));
                }
                
                editorViewport->IsMouseOver = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                
                auto renderTarget = editorViewport->FrameBuffer->GetCurrentRenderTarget();
                ImGui::Image(renderTarget->GetGPUAddress(), viewportSize);
            }
            ImGui::End();
        }
    };
}
