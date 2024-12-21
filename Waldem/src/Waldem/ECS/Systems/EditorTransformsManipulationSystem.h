#pragma once
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "System.h"
#include "glm/gtc/type_ptr.hpp"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API EditorTransformsManipulationSystem : ISystem
    {
        Window* Window;
        ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
        
    public:
        EditorTransformsManipulationSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager) override
        {
            Window = sceneData->Window;
        }

        void Update(float deltaTime) override
        {
            ImGuizmo::SetOrthographic(false);

            auto windowPos = Window->GetPosition();
            ImGuizmo::SetRect(windowPos.x, windowPos.y, Window->GetWidth(), Window->GetHeight());
            for (auto [cameraEntity, camera, cameraTransform, mainCamera] : ECSManager->EntitiesWith<Camera, Transform, MainCamera>())
            {
                for (auto [transformEntity, transform, selected] : ECSManager->EntitiesWith<Transform, Selected>())
                {
                    ImGuizmo::Manipulate(value_ptr(camera.ViewMatrix), value_ptr(camera.ProjectionMatrix), currentOperation, ImGuizmo::LOCAL, value_ptr(transform.Matrix));
                }
            }
        }
    };
}
