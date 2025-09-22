#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "Widget.h"
#include "glm/gtc/type_ptr.hpp"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem
{
    class WALDEM_API EditorViewportWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGuizmo::OPERATION CurrentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE CurrentMode = ImGuizmo::WORLD;
        bool CanModifyManipulationSettings = false;
        bool IsVisible = false;
        
    public:
        EditorViewportWidget() {}

        void Initialize(InputManager* inputManager) override
        {
            
            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_RIGHT, [&](bool isPressed)
            {
                //when we control camera with RMB we can't change operation
                CanModifyManipulationSettings = !isPressed;
            });

            inputManager->SubscribeToKeyEvent(W, [&](bool isPressed)
            {
                if(isPressed && CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::TRANSLATE;
                }
            });

            inputManager->SubscribeToKeyEvent(E, [&](bool isPressed)
            {
                if(isPressed && CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::ROTATE;
                }
            });

            inputManager->SubscribeToKeyEvent(R, [&](bool isPressed)
            {
                if(isPressed && CanModifyManipulationSettings)
                {
                    CurrentOperation = ImGuizmo::SCALE;
                }
            });

            inputManager->SubscribeToKeyEvent(Q, [&](bool isPressed)
            {
                if(isPressed && CanModifyManipulationSettings)
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

                    if(IdManager::GetEntityById(Editor::HoveredEntityID, GlobalDrawIdType, outEntity))
                    {
                        if(outEntity.is_valid() && outEntity.is_alive())
                        {
                            outEntity.add<Selected>();
                        }
                    }
                }
            });

            ECS::World.system<Transform, Selected>("EditorGizmoSystem").kind(flecs::OnGUI).each([&](flecs::entity entity, Transform& transform, Selected)
            {
                if(IsVisible)
                {
                    auto editorViewport = ViewportManager::GetEditorViewport();
                    
                    ECS::Entity linkedCamera;
                    if(editorViewport->TryGetLinkedCamera(linkedCamera))
                    {
                        if(auto editorCamera = linkedCamera.get<Camera>())
                        {
                            ImGuizmo::SetOrthographic(false);
                            ImGuizmo::SetRect(editorViewport->Position.x, editorViewport->Position.y, editorViewport->Size.x, editorViewport->Size.y);
                            ImGuizmo::Manipulate(value_ptr(editorCamera->ViewMatrix), value_ptr(editorCamera->ProjectionMatrix), CurrentOperation, CurrentMode, value_ptr(transform.Matrix));

                            if(ImGuizmo::IsUsing())
                            {
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

                    ECS::Entity linkedCamera;
                    if(viewport->TryGetLinkedCamera(linkedCamera))
                    {
                        auto camera = linkedCamera.get_mut<Camera>();
                        camera->UpdateProjectionMatrix(camera->FieldOfView, viewportSize.x/viewportSize.y, camera->NearPlane, camera->FarPlane);
                        linkedCamera.modified<Camera>(); 
                    }
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
