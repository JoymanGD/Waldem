#pragma once
#include "imgui.h"
#include "Widget.h"
#include "../EditorShortcutContext.h"
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
            const bool isVisible = ImGui::Begin("Game###Game", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            if (isVisible)
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
                    "Radiance",
                    "NIV Irradiance",
                    "Path Tracing"
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
                if(viewport->IsMouseOver && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
                {
                    viewport->IsFocused = true;
                    if(auto editorViewport = ViewportManager::GetEditorViewport())
                    {
                        editorViewport->IsFocused = false;
                    }
                }
                EditorShortcutContexts::SetActive(EditorShortcutContext::GameViewport, viewport->IsFocused);
                
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
                    ImGui::Checkbox("NIV Inference##Game", &toggles.EnableNIVInference);
                    ImGui::Checkbox("NIV Temporal##Game", &Renderer::RenderData.EnableNIVTemporalSmoothing);
                    ImGui::Checkbox("NIV Spatial##Game", &Renderer::RenderData.EnableNIVSpatialFilter);
                    ImGui::SliderFloat("NIV History##Game", &Renderer::RenderData.NIVTemporalHistoryWeight, 0.0f, 0.98f, "%.2f");
                    ImGui::SliderFloat("NIV Filter##Game", &Renderer::RenderData.NIVSpatialFilterStrength, 0.0f, 1.0f, "%.2f");
                    ImGui::Separator();
                    ImGui::Checkbox("Path Tracing##Game", &toggles.EnablePathTracing);
                    ImGui::Checkbox("PT Accumulation##Game", &toggles.EnablePathTracingAccumulation);
                    int ptBounces = (int)toggles.PathTracingMaxBounces;
                    int ptSpp = (int)toggles.PathTracingSamplesPerPixel;
                    if(ImGui::SliderInt("PT Bounces##Game", &ptBounces, 1, 4))
                    {
                        toggles.PathTracingMaxBounces = (uint)ptBounces;
                    }
                    if(ImGui::SliderInt("PT SPP##Game", &ptSpp, 1, 8))
                    {
                        toggles.PathTracingSamplesPerPixel = (uint)ptSpp;
                    }
                    ImGui::EndPopup();
                }

                Renderer::RenderData.GameViewportOutputTarget = SelectedRenderTarget;
            }
            else
            {
                viewport->IsMouseOver = false;
                viewport->IsFocused = false;
                EditorShortcutContexts::SetActive(EditorShortcutContext::GameViewport, false);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}


