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
                    ImGui::Separator();
                    auto& renderData = Renderer::RenderData;
                    int trainingSampleCount = (int)renderData.TrainingDatasetSampleCount;
                    int trainingRaysPerPoint = (int)renderData.TrainingDatasetRaysPerPoint;
                    int trainingBatches = (int)renderData.TrainingDatasetCaptureBatches;
                    int trainingBounces = (int)renderData.TrainingDatasetMaxBounces;
                    int trainingSeed = (int)renderData.TrainingDatasetSeed;
                    if(ImGui::InputInt("Train Samples##Game", &trainingSampleCount))
                    {
                        renderData.TrainingDatasetSampleCount = trainingSampleCount > 1 ? (uint)trainingSampleCount : 1;
                    }
                    if(ImGui::SliderInt("Train Rays/Point##Game", &trainingRaysPerPoint, 1, 256))
                    {
                        renderData.TrainingDatasetRaysPerPoint = (uint)trainingRaysPerPoint;
                    }
                    if(ImGui::SliderInt("Train Batches##Game", &trainingBatches, 1, 128))
                    {
                        renderData.TrainingDatasetCaptureBatches = (uint)trainingBatches;
                    }
                    if(ImGui::SliderInt("Train Bounces##Game", &trainingBounces, 1, 4))
                    {
                        renderData.TrainingDatasetMaxBounces = (uint)trainingBounces;
                    }
                    if(ImGui::InputInt("Train Seed##Game", &trainingSeed))
                    {
                        renderData.TrainingDatasetSeed = trainingSeed > 0 ? (uint)trainingSeed : 1;
                    }
                    ImGui::Checkbox("Train Debug Output##Game", &renderData.TrainingDatasetDebugOutput);
                    ImGui::TextWrapped("Output: %s", renderData.TrainingDatasetOutputPath.C_Str());
                    if(ImGui::Button("Capture Training Dataset##Game"))
                    {
                        renderData.RequestTrainingDatasetCapture = true;
                    }
                    ImGui::EndPopup();
                }

                Renderer::RenderData.GameViewportOutputTarget = SelectedRenderTarget;
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}
