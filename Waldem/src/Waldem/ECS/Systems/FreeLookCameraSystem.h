#pragma once
#include "System.h"
#include "Waldem/Application.h"
#include "Waldem/Input/Input.h"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API FreeLookCameraSystem : ISystem
    {
        Vector2 MousePos = { 0, 0 };
        Vector2 LastMousePos = { 0, 0 };
        bool IsUnderControl = false;
        Vector3 DeltaPos = { 0, 0, 0 };
        
    public:
        FreeLookCameraSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager) override
        {
            inputManager->SubscribeToKeyEvent(W, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(0, 0, 1) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(S, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(0, 0, -1) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(A, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(-1, 0, 0) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(D, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(1, 0, 0) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(E, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(0, 1, 0) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(Q, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(0, -1, 0) * multiplier;
            });

            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_LEFT, [&](bool isPressed)
            {
                IsUnderControl = isPressed;
            });

            inputManager->SubscribeToMouseMoveEvent([&](Vector2 mousePos)
            {
                MousePos = mousePos;
            });

            for (auto [entity, transform, camera, mainCamera] : ECSManager->EntitiesWith<Transform, Camera, MainCamera>())
            {
                inputManager->SubscribeToMouseScrollEvent([&](Vector2 scroll)
                {
                    if(IsUnderControl)
                    {
                        camera.SpeedModificator += scroll.y * camera.SpeedParams.ModificationStep;
                        camera.SpeedModificator = std::clamp(camera.SpeedModificator, camera.SpeedParams.MinSpeedModificator, camera.SpeedParams.MaxSpeedModificator);
                    }
                });
            }
        }

        void Update(float deltaTime) override
        {
            for (auto [entity, transform, camera, mainCamera] : ECSManager->EntitiesWith<Transform, Camera, MainCamera>())
            {               
                if (IsUnderControl)
                {
                    float deltaX = (MousePos.x - LastMousePos.x) * deltaTime;
                    float deltaY = (MousePos.y - LastMousePos.y) * deltaTime;

                    transform.Rotate(deltaX * camera.RotationSpeed, deltaY * camera.RotationSpeed, 0);

                    if(DeltaPos != Vector3(0, 0, 0))
                    {
                        transform.Move(normalize(DeltaPos) * deltaTime * camera.MovementSpeed * camera.SpeedModificator);
                    }
                }
                
                LastMousePos.x = MousePos.x;
                LastMousePos.y = MousePos.y;
                
                camera.SetViewMatrix(&transform);
            }
        }
    };
}