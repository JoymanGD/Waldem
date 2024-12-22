#pragma once

#include "ImGuizmo.h"
#include "System.h"
#include "glm/gtc/type_ptr.hpp"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    struct Ray
    {
        Vector3 Origin;
        Vector3 Direction;
    };
    
    class WALDEM_API EditorTransformsManipulationSystem : ISystem
    {
        Window* Window;
        ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
        
    public:
        EditorTransformsManipulationSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager) override
        {
            Window = sceneData->Window;

            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_LEFT, [&](bool isPressed)
            {
                if(!isPressed)
                {
                    // TraceRay();
                }
            });

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

        // void TraceRay()
        // {
        //     for (auto [cameraEntity, camera, cameraTransform, mainCamera] : ECSManager->EntitiesWith<Camera, Transform, MainCamera>())
        //     {
        //         for (auto [transformEntity, model] : ECSManager->EntitiesWith<ModelComponent>())
        //         {
        //             model.Model.
        //         }
        //     }
        // }
        //
        // bool RayIntersectsAABB(const Ray& ray, const AABB& aabb, float& tMin, float& tMax) {
        //     tMin = 0.0f;
        //     tMax = FLT_MAX;
        //
        //     for (int i = 0; i < 3; i++) {
        //         if (glm::abs(ray.direction[i]) < 1e-8f) {
        //             // Ray is parallel to the slab
        //             if (ray.origin[i] < aabb.min[i] || ray.origin[i] > aabb.max[i]) {
        //                 return false;
        //             }
        //         } else {
        //             float t1 = (aabb.min[i] - ray.origin[i]) / ray.direction[i];
        //             float t2 = (aabb.max[i] - ray.origin[i]) / ray.direction[i];
        //
        //             if (t1 > t2) std::swap(t1, t2);
        //
        //             tMin = glm::max(tMin, t1);
        //             tMax = glm::min(tMax, t2);
        //
        //             if (tMin > tMax) return false;
        //         }
        //     }
        //     return true;
        // }
    };
}