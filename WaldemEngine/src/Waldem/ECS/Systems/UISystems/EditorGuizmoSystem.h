#pragma once

#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/Renderer/Viewport.h"

namespace Waldem
{
    class WALDEM_API EditorGuizmoSystem : public ISystem
    {
        ImGuizmo::OPERATION CurrentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE CurrentMode = ImGuizmo::WORLD;
        bool CanModifyManipulationSettings = false;
        
    public:
        EditorGuizmoSystem() {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
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

                    auto entityId = Editor::GetEntityID(Editor::HoveredEntityID);

                    auto entity = ECS::World.entity(entityId);

                    if(entity.is_valid() && entity.is_alive())
                    {
                        entity.add<Selected>();
                    }
                }
            });

            ECS::World.system<Transform, Selected>("EditorGizmoSystem").kind(flecs::OnGUI).each([&](flecs::entity entity, Transform& transform, Selected)
            {
                if(auto editorCameraEntity = ECS::World.lookup("EditorCamera"))
                {
                    if(auto editorCamera = editorCameraEntity.get<Camera>())
                    {
                        auto editorViewport = Renderer::GetEditorViewport();

                        ImGuizmo::SetOrthographic(false);
                        ImGuizmo::SetRect(editorViewport->Position.x, editorViewport->Position.y, editorViewport->Size.x, editorViewport->Size.y);
                        Matrix4 transformMatrix = translate(Matrix4(1.0f), transform.Position) * Matrix4(1.0f) * scale(Matrix4(1.0f), transform.LocalScale);
                        ImGuizmo::Manipulate(value_ptr(editorCamera->ViewMatrix), value_ptr(editorCamera->ProjectionMatrix), CurrentOperation, CurrentMode, value_ptr(transformMatrix));

                        if(ImGuizmo::IsUsing())
                        {
                            Quaternion rotationQuat;
                            Vector3 skew;
                            Vector4 perspective;

                            decompose(transformMatrix, transform.LocalScale, rotationQuat, transform.Position, skew, perspective);

                            transform.Rotate(rotationQuat);
                        }
                        
                        transform.Update();
                    }
                }
            });
        }
    };
}
