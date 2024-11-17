#pragma once
#include "System.h"
#include "Waldem/Input.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API DebugSystem : ISystem
    {
        Vector3 LightTargetPosition = { 0, -1, 0 };
        
    public:
        DebugSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override {}

        void Update(SceneData* sceneData, float deltaTime) override
        {
            Vector3 cameraForward, cameraRight;
            for (auto [entity, camera, transform] : ECSManager->EntitiesWith<Camera, Transform>())
            {
                cameraForward = transform.GetForwardVector();
                cameraRight = transform.GetRightVector();
            }
            
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                static float lastMouseX = 0;
                static float lastMouseY = 0;
                
                auto [mouseX, mouseY] = Input::GetMousePos();

                if (Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_RIGHT))
                {
                    cameraForward.y = 0;
                    cameraRight.y = 0;
                    
                    float deltaX = (mouseX - lastMouseX) * deltaTime;
                    float deltaY = (mouseY - lastMouseY) * deltaTime;

                    LightTargetPosition += normalize(cameraRight)*deltaX + normalize(cameraForward)*-deltaY;

                    transform.LookAt(LightTargetPosition);
                }
                
                lastMouseX = mouseX;
                lastMouseY = mouseY;
            }
        }
    };
}
