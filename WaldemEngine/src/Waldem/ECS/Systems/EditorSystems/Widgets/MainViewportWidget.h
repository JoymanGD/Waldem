#pragma once
#include "Waldem/ECS/Systems/EditorSystems/Widgets/IWidgetContainerSystem.h"
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

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 regionMin = ImGui::GetWindowContentRegionMin();
                ImVec2 regionMax = ImGui::GetWindowContentRegionMax();

                // Position of top-left of content region in screen space
                ImVec2 gizmoPos = ImVec2(windowPos.x + regionMin.x, windowPos.y + regionMin.y);
                ImVec2 gizmoSize = ImVec2(regionMax.x - regionMin.x, regionMax.y - regionMin.y);
                Editor::EditorViewportSize = Vector2(gizmoSize.x, gizmoSize.y);
                Editor::EditorViewportPos = Vector2(gizmoPos.x, gizmoPos.y);
                Editor::IsMouseOverEditorViewport = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                auto renderTarget = Renderer::GetEditorViewport()->FrameBuffer->GetCurrentRenderTarget();
                // ImGui::Image(renderTarget->GetPlatformShaderResourceHandle(), ImVec2(renderTarget->GetWidth(), renderTarget->GetHeight()));
                ImGui::Image(renderTarget->GetPlatformShaderResourceHandle(), gizmoSize);
            }
            ImGui::End();
        }
    };
}
