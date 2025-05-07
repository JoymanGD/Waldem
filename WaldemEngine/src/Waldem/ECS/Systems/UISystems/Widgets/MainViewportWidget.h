#pragma once
#include "Waldem/ECS/Systems/UISystems/Widgets/IWidgetContainerSystem.h"
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
        MainViewportWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        WString GetName() override { return "Viewport"; }

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
                
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 regionMin = ImGui::GetWindowContentRegionMin();
                ImVec2 regionMax = ImGui::GetWindowContentRegionMax();

                // Position of top-left of content region in screen space
                ImVec2 viewportPos = ImVec2(windowPos.x + regionMin.x, windowPos.y + regionMin.y);
                ImVec2 viewportSize = ImVec2(regionMax.x - regionMin.x, regionMax.y - regionMin.y);

                if(viewportSize.x != Editor::EditorViewportSize.x || viewportSize.y != Editor::EditorViewportSize.y)
                {
                    Editor::Resize(Vector2(viewportSize.x, viewportSize.y));
                }
                
                Editor::EditorViewportSize = Vector2(viewportSize.x, viewportSize.y);
                Editor::EditorViewportPos = Vector2(viewportPos.x, viewportPos.y);
                Editor::IsMouseOverEditorViewport = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                Editor::EditorViewportMousePos = Vector2(ImGui::GetMousePos().x - viewportPos.x, ImGui::GetMousePos().y - viewportPos.y);
                
                auto renderTarget = Renderer::GetEditorViewport()->FrameBuffer->GetCurrentRenderTarget();
                ImGui::Image(renderTarget->GetPlatformShaderResourceHandle(), viewportSize);
            }
            ImGui::End();
        }
    };
}
