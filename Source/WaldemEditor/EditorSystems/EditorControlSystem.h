#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Engine.h"
#include "Waldem/Time.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/MouseButtonCodes.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem
{
    class EditorCameraControlSystem : public ISystem
    {
        //Camera control
        Vector2 MousePos = { 0, 0 };
        Vector2 LastMousePos = { 0, 0 };
        bool IsUnderControl = false;
        Vector3 DeltaPos = { 0, 0, 0 };
        float DeltaScroll = 0;

    public:
        EditorCameraControlSystem() {}

        void Initialize(InputManager* inputManager) override
        {
            InitializeCameraControl(inputManager);
            InitializeCameras();
            
            UpdateCameraControl();
            UpdateLastMousePosition();
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
            
            inputManager->SubscribeToMouseScrollEvent([&](Vector2 scroll)
            {
                if(IsUnderControl)
                {
                    DeltaScroll = scroll.y;
                }
            });
        }
        
        void InitializeCameras()
        {
            ECS::World.query("InitializeCamerasSystem").each([&]
            {
                ECS::Entity linkedCamera;
                
                if(ViewportManager::GetEditorViewport()->TryGetLinkedCamera(linkedCamera))
                {
                    auto& camera = linkedCamera.get_mut<Camera>();
                    auto transform = linkedCamera.get_mut<Transform>();
                    camera.SetViewMatrix(transform);

                    linkedCamera.modified<Transform>();
                }
            });
        }

        void UpdateLastMousePosition()
        {
            ECS::World.system("UpdateLastMousePositionSystem").kind(ECS::OnUpdate).each([&]
            {
                LastMousePos.x = MousePos.x;
                LastMousePos.y = MousePos.y;
            });
        }
        
        void UpdateCameraControl()
        {
            ECS::World.system("UpdateCameraControlSystem").kind(ECS::OnUpdate).each([&]
            {
                ECS::Entity linkedCamera;
                if(ViewportManager::GetEditorViewport()->TryGetLinkedCamera(linkedCamera))
                {
                    auto& camera = linkedCamera.get_mut<Camera>();
                    auto& transform = linkedCamera.get_mut<Transform>();
                    
                    if (IsUnderControl)
                    {
                        float deltaX = (MousePos.x - LastMousePos.x) * Time::DeltaTime;
                        float deltaY = (MousePos.y - LastMousePos.y) * Time::DeltaTime;

                        transform.Rotate(deltaY * camera.RotationSpeed, deltaX * camera.RotationSpeed, 0.f);

                        if(DeltaPos != Vector3(0, 0, 0))
                        {
                            transform.Move(normalize(DeltaPos) * Time::DeltaTime * camera.MovementSpeed * camera.SpeedModificator, Local);
                        }
                    
                        camera.SetViewMatrix(transform);
                        
                        linkedCamera.modified<Camera>();
                    }
                }
            });

            ECS::World.system("UpdateCameraSpeedControlSystem").kind(ECS::OnUpdate).each([&]
            {
                ECS::Entity linkedCamera;
                if(ViewportManager::GetEditorViewport()->TryGetLinkedCamera(linkedCamera))
                {
                    if(DeltaScroll != 0.f)
                    {
                        auto& camera = linkedCamera.get_mut<Camera>();
                        camera.SpeedModificator += DeltaScroll * camera.SpeedParams.ModificationStep;
                        camera.SpeedModificator = std::clamp(camera.SpeedModificator, camera.SpeedParams.MinSpeedModificator, camera.SpeedParams.MaxSpeedModificator);
                        linkedCamera.modified<Camera>();
                        
                        DeltaScroll = 0;
                    }
                }
            });
        }
    };
}
