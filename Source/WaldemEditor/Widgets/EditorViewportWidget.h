#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "Widget.h"
#include "glm/gtc/type_ptr.hpp"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Commands/EditorCommands.h"
#include "../EditorShortcutContext.h"
#include "../EditorShortcuts.h"

namespace Waldem
{
    class EditorViewportWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGuizmo::OPERATION CurrentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE CurrentMode = ImGuizmo::WORLD;
        bool CanModifyManipulationSettings = false;
        bool IsVisible = false;
        bool WasUsingGizmo = false;
        ComponentValueBlob GizmoBeforeTransform;
        int SelectedRenderTarget = 0;
        
    public:
        EditorViewportWidget() {}

        void Initialize(InputManager* inputManager) override
        {
            
            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_RIGHT, [&](bool isPressed)
            {
                //when we control camera with RMB we can't change operation
                CanModifyManipulationSettings = !isPressed;
            });

            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::GizmoTranslate, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::TRANSLATE;
                }
            }, [] { return EditorShortcutContexts::Has(EditorShortcutContext::EditorViewport); });

            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::GizmoRotate, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::ROTATE;
                }
            }, [] { return EditorShortcutContexts::Has(EditorShortcutContext::EditorViewport); });

            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::GizmoScale, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::SCALE;
                }
            }, [] { return EditorShortcutContexts::Has(EditorShortcutContext::EditorViewport); });

            inputManager->SubscribeToEditorShortcut(EditorShortcutAction::GizmoToggleMode, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentMode = CurrentMode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
            }, [] { return EditorShortcutContexts::Has(EditorShortcutContext::EditorViewport); });

            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_LEFT, [&](bool isPressed)
            {
                if (isPressed)
                {
                    ECS::World.defer([&]
                    {
                        ECS::World.query<Transform, Selected>("ClearSelectionQuery").each([&](ECS::Entity entity, Transform&, Selected)
                        {
                            entity.remove<Selected>();
                        });
                    });

                    ECS::Entity outEntity;

                    if(IdManager::GetEntityById(EntitySelectionSystem::HoveredEntityID, (IdType)EntitySelectionSystem::HoveredEntityType, outEntity))
                    {
                        if(outEntity.is_valid() && outEntity.is_alive())
                        {
                            outEntity.add<Selected>();
                        }
                    }
                }
            });

            ECS::World.system<Transform, Selected>("EditorTransformGizmoSystem").kind<ECS::OnGUI>().each([&](ECS::Entity entity, Transform& transform, Selected)
            {
                if(IsVisible)
                {
                    auto editorViewport = ViewportManager::GetEditorViewport();
                    
                    ECS::Entity linkedCamera;
                    if(editorViewport->TryGetLinkedCamera(linkedCamera))
                    {
                        if(linkedCamera.has<Camera>())
                        {
                            auto& editorCamera = linkedCamera.get<Camera>();
                            ImGuizmo::SetOrthographic(false);
                            ImGuizmo::SetRect(editorViewport->Position.x, editorViewport->Position.y, editorViewport->Size.x, editorViewport->Size.y);
                            ImGuizmo::Manipulate(&editorCamera.ViewMatrix[0][0], &editorCamera.ProjectionMatrix[0][0], CurrentOperation, CurrentMode, &transform.Matrix[0][0]);

                            if(ImGuizmo::IsUsing())
                            {
                                if(!WasUsingGizmo)
                                {
                                    GizmoBeforeTransform.Capture(ECS::World.id<Transform>(), &transform);
                                }

                                WasUsingGizmo = true;
                                transform.DecompileMatrix();
                                
                                entity.modified<Transform>();
                            }
                            else
                            {
                                if(transform.LastRotation != transform.Rotation)
                                {
                                    transform.ApplyPitchYawRoll();
                                    transform.LastRotation = transform.Rotation;
                                    transform.Update();
                                    entity.modified<Transform>();
                                }

                                if(transform.LastPosition != transform.Position || transform.LastScale != transform.LocalScale)
                                {
                                    transform.LastPosition = transform.Position;
                                    transform.LastScale = transform.LocalScale;
                                    transform.Update();
                                    entity.modified<Transform>();
                                }

                                if(WasUsingGizmo)
                                {
                                    ComponentValueBlob after(ECS::World.id<Transform>(), &transform);
                                    if(GizmoBeforeTransform.IsValid() && after.IsValid() && !GizmoBeforeTransform.Equals(after))
                                    {
                                        EditorCommandHistory::Get().Execute(std::make_unique<SetComponentDataCommand>(
                                            entity.id(),
                                            ECS::World.id<Transform>(),
                                            GizmoBeforeTransform,
                                            after,
                                            false
                                        ));
                                    }

                                    WasUsingGizmo = false;
                                    GizmoBeforeTransform = {};
                                }
                            }
                        }
                    }
                }
            });
        }

        void OnDraw(float deltaTime) override
        {
            auto viewport = ViewportManager::GetEditorViewport();
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            IsVisible = ImGui::Begin("Editor###Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            if (IsVisible)
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

                ImGuizmo::BeginFrame();
                ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
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
                    if(auto gameViewport = ViewportManager::GetGameViewport())
                    {
                        gameViewport->IsFocused = false;
                    }
                }
                EditorShortcutContexts::SetActive(EditorShortcutContext::EditorViewport, viewport->IsFocused);
                
                auto renderTarget = viewport->FrameBuffer->GetCurrentRenderTarget();
                ImGui::Image(renderTarget->GetGPUAddress(), viewportSize);

                const float buttonWidth = 26.0f;
                const float buttonHeight = 22.0f;
                const float buttonPadding = 8.0f;
                ImGui::SetCursorScreenPos(ImVec2(
                    viewportPos.x + viewportSize.x - buttonWidth - buttonPadding,
                    viewportPos.y + buttonPadding));

                if(ImGui::Button("...##EditorViewportOptions", ImVec2(buttonWidth, buttonHeight)))
                {
                    ImGui::OpenPopup("EditorViewportOptionsPopup");
                }

                if(ImGui::BeginPopup("EditorViewportOptionsPopup"))
                {
                    ImGui::SetNextItemWidth(180.0f);
                    ImGui::Combo("Render Target", &SelectedRenderTarget, renderTargets, IM_ARRAYSIZE(renderTargets));
                    ImGui::Separator();

                    auto& toggles = Renderer::RenderData.FeatureToggles;
                    ImGui::Checkbox("Sky##Editor", &toggles.EnableSkyPass);
                    ImGui::Checkbox("GBuffer##Editor", &toggles.EnableGBufferPass);
                    ImGui::Checkbox("Ray Tracing##Editor", &toggles.EnableRayTracingPass);
                    ImGui::Checkbox("Reflections##Editor", &toggles.EnableReflections);
                    ImGui::Checkbox("Direct Lighting##Editor", &toggles.EnableDirectLighting);
                    ImGui::Checkbox("Specular##Editor", &toggles.EnableSpecular);
                    ImGui::Checkbox("Metallic##Editor", &toggles.EnableMetallic);
                    ImGui::Checkbox("NIV Inference##Editor", &toggles.EnableNIVInference);
                    ImGui::Checkbox("NIV Temporal##Editor", &Renderer::RenderData.EnableNIVTemporalSmoothing);
                    ImGui::Checkbox("NIV Spatial##Editor", &Renderer::RenderData.EnableNIVSpatialFilter);
                    ImGui::SliderFloat("NIV History##Editor", &Renderer::RenderData.NIVTemporalHistoryWeight, 0.0f, 0.98f, "%.2f");
                    ImGui::SliderFloat("NIV Filter##Editor", &Renderer::RenderData.NIVSpatialFilterStrength, 0.0f, 1.0f, "%.2f");
                    ImGui::Separator();
                    ImGui::Checkbox("Path Tracing##Editor", &toggles.EnablePathTracing);
                    ImGui::Checkbox("PT Accumulation##Editor", &toggles.EnablePathTracingAccumulation);
                    int ptBounces = (int)toggles.PathTracingMaxBounces;
                    int ptSpp = (int)toggles.PathTracingSamplesPerPixel;
                    if(ImGui::SliderInt("PT Bounces##Editor", &ptBounces, 1, 4))
                    {
                        toggles.PathTracingMaxBounces = (uint)ptBounces;
                    }
                    if(ImGui::SliderInt("PT SPP##Editor", &ptSpp, 1, 8))
                    {
                        toggles.PathTracingSamplesPerPixel = (uint)ptSpp;
                    }
                    ImGui::EndPopup();
                }

                Renderer::RenderData.EditorViewportOutputTarget = SelectedRenderTarget;
            }
            else
            {
                viewport->IsMouseOver = false;
                viewport->IsFocused = false;
                EditorShortcutContexts::SetActive(EditorShortcutContext::EditorViewport, false);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}


