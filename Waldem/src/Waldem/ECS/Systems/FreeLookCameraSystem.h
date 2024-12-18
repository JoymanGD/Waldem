#pragma once
#include "System.h"
#include "Waldem/Input/Input.h"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API FreeLookCameraSystem : ISystem
    {
    public:
        FreeLookCameraSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override {}

        void Update(float deltaTime) override
        {
            for (auto [entity, transform, camera] : ECSManager->EntitiesWith<Transform, Camera>())
            {
                if(Input::IsKeyPressed(W))
                {
                    transform.Move({ 0, 0, 0.1f * deltaTime * camera.MovementSpeed });
                }
                if(Input::IsKeyPressed(S))
                {
                    transform.Move({ 0, 0, -0.1f * deltaTime * camera.MovementSpeed });
                }
                if(Input::IsKeyPressed(A))
                {
                    transform.Move({ -0.1f * deltaTime * camera.MovementSpeed, 0, 0 });
                }
                if(Input::IsKeyPressed(D))
                {
                    transform.Move({ 0.1f * deltaTime * camera.MovementSpeed, 0, 0 });
                }
                if(Input::IsKeyPressed(E))
                {
                    transform.Move({ 0, 0.1f * deltaTime * camera.MovementSpeed, 0 });
                }
                if(Input::IsKeyPressed(Q))
                {
                    transform.Move({ 0, -0.1f * deltaTime * camera.MovementSpeed, 0 });
                }
                
                static float lastMouseX = 0;
                static float lastMouseY = 0;
                
		        auto [mouseX, mouseY] = Input::GetMousePos();

                if (Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_LEFT))
                {
                    float deltaX = (mouseX - lastMouseX) * deltaTime;
                    float deltaY = (mouseY - lastMouseY) * deltaTime;

                    transform.Rotate(deltaX * camera.RotationSpeed, deltaY * camera.RotationSpeed, 0);
                }
                
                lastMouseX = mouseX;
                lastMouseY = mouseY;

                camera.SetViewMatrix(&transform);
            }
        }
    };
}
