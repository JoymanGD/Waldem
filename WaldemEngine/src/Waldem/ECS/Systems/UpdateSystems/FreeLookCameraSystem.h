#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "..\..\..\Engine.h"
#include "Waldem/Input/Input.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"

namespace Waldem
{
    class WALDEM_API FreeLookCameraSystem : public ISystem
    {
        Vector2 MousePos = { 0, 0 };
        Vector2 LastMousePos = { 0, 0 };
        bool IsUnderControl = false;
        Vector3 DeltaPos = { 0, 0, 0 };
        
    public:
        FreeLookCameraSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
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

            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_RIGHT, [&](bool isPressed)
            {
                IsUnderControl = isPressed;
            });

            inputManager->SubscribeToMouseMoveEvent([&](Vector2 mousePos)
            {
                MousePos = mousePos;
            });

            for (auto [entity, transform, camera, mainCamera] : Manager->EntitiesWith<Transform, Camera, EditorCamera>())
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
            for (auto [entity, transform, camera, mainCamera] : Manager->EntitiesWith<Transform, Camera, EditorCamera>())
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