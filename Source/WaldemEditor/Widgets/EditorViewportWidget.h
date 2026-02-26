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
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Commands/EditorCommands.h"
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
        
    public:
        EditorViewportWidget() {}

        void Initialize(InputManager* inputManager) override
        {
            
            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_RIGHT, [&](bool isPressed)
            {
                //when we control camera with RMB we can't change operation
                CanModifyManipulationSettings = !isPressed;
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::GizmoTranslate);
            }, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::TRANSLATE;
                }
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::GizmoRotate);
            }, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::ROTATE;
                }
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::GizmoScale);
            }, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::SCALE;
                }
            });

            inputManager->SubscribeToDynamicShortcut([]
            {
                return EditorShortcuts::GetShortcut(EditorShortcutAction::GizmoToggleMode);
            }, [&]
            {
                if(CanModifyManipulationSettings)
                {
                    CurrentMode = CurrentMode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
            });

            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_LEFT, [&](bool isPressed)
            {
                if (isPressed)
                {
                    ECS::World.defer([&]
                    {
                        ECS::World.query<Transform, Selected>("ClearSelectionQuery").each([&](flecs::entity entity, Transform&, Selected)
                        {
                            entity.remove<Selected>();
                        });
                    });

                    flecs::entity outEntity;

                    if(IdManager::GetEntityById(EntitySelectionSystem::HoveredEntityID, (IdType)EntitySelectionSystem::HoveredEntityType, outEntity))
                    {
                        if(outEntity.is_valid() && outEntity.is_alive())
                        {
                            outEntity.add<Selected>();
                        }
                    }
                }
            });

            ECS::World.system<Transform, Selected>("EditorTransformGizmoSystem").kind<ECS::OnGUI>().each([&](flecs::entity entity, Transform& transform, Selected)
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
            IsVisible = ImGui::Begin(viewport->Name, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            if (IsVisible)
            {
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
                
                auto renderTarget = viewport->FrameBuffer->GetCurrentRenderTarget();
                ImGui::Image(renderTarget->GetGPUAddress(), viewportSize);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
}
