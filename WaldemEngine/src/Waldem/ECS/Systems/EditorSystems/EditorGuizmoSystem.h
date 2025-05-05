#pragma once

#include "ImGuizmo.h"
#include "glm/gtc/type_ptr.hpp"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Camera.h"

namespace Waldem
{
    class WALDEM_API EditorGuizmoSystem : public ISystem
    {
        CWindow* Window;
        ImGuizmo::OPERATION CurrentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE CurrentMode = ImGuizmo::WORLD;
        bool CanModifyManipulationSettings = false;
        
    public:
        EditorGuizmoSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Window = sceneData->Window;

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
                if(isPressed)
                {
                    for (auto [entity, transform, selected] : Manager->EntitiesWith<Transform, Selected>())
                    {
                        entity.Remove<Selected>();
                    }
                    
                    int meshId = 0;
                    
                    for (auto [entity, mesh, transform] : Manager->EntitiesWith<MeshComponent, Transform>())
                    {
                        if(meshId == Editor::HoveredEntityID)
                        {
                            entity.Add<Selected>();
                            break;
                        }
                        meshId++;
                    }
                }
            });
        }

        void Update(float deltaTime) override
        {
            ImGuizmo::SetOrthographic(false);

            ImGuizmo::SetRect(Editor::EditorViewportPos.x, Editor::EditorViewportPos.y, Editor::EditorViewportSize.x, Editor::EditorViewportSize.y);
            for (auto [cameraEntity, camera, cameraTransform, mainCamera] : Manager->EntitiesWith<Camera, Transform, EditorCamera>())
            {
                for (auto [transformEntity, transform, selected] : Manager->EntitiesWith<Transform, Selected>())
                {
                    ImGuizmo::Manipulate(value_ptr(camera.ViewMatrix), value_ptr(camera.ProjectionMatrix), CurrentOperation, CurrentMode, value_ptr(transform.Matrix));
                    transform.DecompileMatrix();
                }
            }
        }
    };
}