#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Engine.h"
#include "Waldem/Input/Input.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Light.h"

namespace Waldem
{
    class WALDEM_API EditorControlSystem : public ISystem
    {
        //Camera control
        Vector2 MousePos = { 0, 0 };
        Vector2 LastMousePos = { 0, 0 };
        bool IsUnderControl = false;
        Vector3 DeltaPos = { 0, 0, 0 };

        //Light control
        Vector3 LightTargetPosition = { 0, -1, 0 };
        Vector3 LightTargetDirection = { 0, -1, 0 };
        bool IsRotatingLight = false;
        
    public:
        EditorControlSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
            InitializeCameraControl(inputManager);
            InitializeLightControl(inputManager);
        }        

        void InitializeCameraControl(InputManager* inputManager)
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

        void InitializeLightControl(InputManager* inputManager)
        {
            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_MIDDLE, [&](bool isPressed)
            {
                IsRotatingLight = isPressed;
            });
        }

        void Update(float deltaTime) override
        {
            UpdateCameraControl(deltaTime);
            UpdateLightControl(deltaTime);
                
            LastMousePos.x = MousePos.x;
            LastMousePos.y = MousePos.y;
        }

        void UpdateLightControl(float deltaTime)
        {
            for (auto [entity, light, transform] : Manager->EntitiesWith<Light, Transform>())
            {
                if(light.Data.Type == LightType::Directional)
                {
                    Vector3 cameraUp, cameraRight;
                    for (auto [cameraEntity, camera, cameraTransform] : Manager->EntitiesWith<Camera, Transform>())
                    {
                        cameraUp = cameraTransform.GetUpVector();
                        cameraRight = cameraTransform.GetRightVector();
                    }
                    
                    if (IsRotatingLight)
                    {
                        cameraRight.y = 0;
                        cameraUp.y = 0;
                    
                        float deltaX = (MousePos.x - LastMousePos.x) * deltaTime;
                        float deltaY = (MousePos.y - LastMousePos.y) * deltaTime;

                        Matrix4 rotationMatrix = rotate(Matrix4(1.0f), deltaX, cameraUp) * rotate(Matrix4(1.0f), deltaY, cameraRight);
                        LightTargetDirection = normalize(Vector3(rotationMatrix * Vector4(LightTargetDirection, 0.0f)));

                        transform.LookAt(transform.Position + LightTargetDirection);
                    }
                }
            }
        }
        
        void UpdateCameraControl(float deltaTime)
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
                
                camera.SetViewMatrix(&transform);
            }
        }
    };
}