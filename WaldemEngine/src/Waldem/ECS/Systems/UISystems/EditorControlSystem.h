#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Engine.h"
#include "Waldem/Time.h"
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
        float DeltaScroll = 0;

        //Light control
        Vector3 LightTargetPosition = { 0, -1, 0 };
        Vector3 LightTargetDirection = { 0, -1, 0 };
        bool IsRotatingLight = false;
        
    public:
        EditorControlSystem() {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            InitializeCameraControl(inputManager);
            InitializeLightControl(inputManager);
            InitializeCameras();
            
            UpdateCameraControl();
            UpdateLightControl();
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

        void InitializeLightControl(InputManager* inputManager)
        {
            inputManager->SubscribeToMouseButtonEvent(WD_MOUSE_BUTTON_MIDDLE, [&](bool isPressed)
            {
                IsRotatingLight = isPressed;
            });
        }
        
        void InitializeCameras()
        {
            ECS::World.query<Camera, Transform>("InitializeCamerasSystem").each([&](flecs::entity entity, Camera& camera, Transform& transform)
            {
                camera.SetViewMatrix(&transform);

                entity.modified<Transform>();
            });
        }

        void UpdateLastMousePosition()
        {
            ECS::World.system<>("UpdateLastMousePositionSystem").kind(flecs::OnUpdate).each([&]
            {
                LastMousePos.x = MousePos.x;
                LastMousePos.y = MousePos.y;
            });
        }
        
        void UpdateLightControl()
        {
            ECS::World.system<Light, Transform, EditorComponent>("UpdateLightControlSystem").kind(flecs::OnUpdate).each([&](flecs::entity entity, Light& light, Transform& transform, EditorComponent)
            {
                if (IsRotatingLight)
                {
                    if(light.Data.Type == LightType::Directional)
                    {
                        Vector3 cameraUp, cameraRight;

                        if(auto editorCamera = ECS::World.lookup("EditorCamera"))
                        {
                            auto cameraTransform = editorCamera.get<Transform>();
                            cameraUp = cameraTransform->GetUpVector();
                            cameraRight = cameraTransform->GetRightVector();
                        }
                        
                        cameraRight.y = 0;
                        cameraUp.y = 0;
                    
                        float deltaX = (MousePos.x - LastMousePos.x) * Time::DeltaTime;
                        float deltaY = (MousePos.y - LastMousePos.y) * Time::DeltaTime;

                        if(deltaX != 0 || deltaY != 0)
                        {
                            Matrix4 rotationMatrix = rotate(Matrix4(1.0f), deltaX, cameraUp) * rotate(Matrix4(1.0f), deltaY, cameraRight);
                            LightTargetDirection = normalize(Vector3(rotationMatrix * Vector4(LightTargetDirection, 0.0f)));

                            transform.LookAt(transform.Position + LightTargetDirection);

                            entity.modified<Transform>();
                        }

                    }
                }
            });
        }
        
        void UpdateCameraControl()
        {
            ECS::World.system<Camera, Transform, EditorComponent>("UpdateCameraControlSystem").kind(flecs::OnUpdate).each([&](flecs::entity entity, Camera& camera, Transform& transform, EditorComponent)
            {
                if (IsUnderControl)
                {
                    float deltaX = (MousePos.x - LastMousePos.x) * Time::DeltaTime;
                    float deltaY = (MousePos.y - LastMousePos.y) * Time::DeltaTime;

                    transform.Rotate(deltaY * camera.RotationSpeed, deltaX * camera.RotationSpeed, 0);

                    if(DeltaPos != Vector3(0, 0, 0))
                    {
                        transform.Move(normalize(DeltaPos) * Time::DeltaTime * camera.MovementSpeed * camera.SpeedModificator);
                    }
                
                    camera.SetViewMatrix(&transform);
                    
                    entity.modified<Camera>();
                }
            });

            ECS::World.system<Camera, EditorComponent>("UpdateCameraSpeedControlSystem").kind(flecs::OnUpdate).each([&](flecs::entity entity, Camera& camera, EditorComponent)
            {
                if(DeltaScroll != 0.f)
                {
                    camera.SpeedModificator += DeltaScroll * camera.SpeedParams.ModificationStep;
                    camera.SpeedModificator = std::clamp(camera.SpeedModificator, camera.SpeedParams.MinSpeedModificator, camera.SpeedParams.MaxSpeedModificator);
                    entity.modified<Camera>();
                    
                    DeltaScroll = 0;
                }
            });
        }
    };
}