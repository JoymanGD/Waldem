#pragma once

#include "ImGuizmo.h"
#include "System.h"
#include "glm/gtc/type_ptr.hpp"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API EditorTransformsManipulationSystem : ISystem
    {
        Window* Window;
        ImGuizmo::OPERATION CurrentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE CurrentMode = ImGuizmo::WORLD;
        bool CanModifyManipulationSettings = false;
        
    public:
        EditorTransformsManipulationSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
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
                    for (auto [entity, mesh, transform, selected] : ECSManager->EntitiesWith<MeshComponent, Transform, Selected>())
                    {
                        entity.Remove<Selected>();
                    }
                    
                    int meshId = 0;
                    
                    for (auto [entity, mesh, transform] : ECSManager->EntitiesWith<MeshComponent, Transform>())
                    {
                        if(meshId == Editor::HoveredIntityID)
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

            ImGuizmo::SetRect(0, 0, Window->GetWidth(), Window->GetHeight());
            for (auto [cameraEntity, camera, cameraTransform, mainCamera] : ECSManager->EntitiesWith<Camera, Transform, MainCamera>())
            {
                for (auto [transformEntity, transform, selected] : ECSManager->EntitiesWith<Transform, Selected>())
                {
                    ImGuizmo::Manipulate(value_ptr(camera.ViewMatrix), value_ptr(camera.ProjectionMatrix), CurrentOperation, CurrentMode, value_ptr(transform.Matrix));
                }
            }
        }
    };
}