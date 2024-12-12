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
        Vector3 LightTargetDirection = { 0, -1, 0 };
        
    public:
        DebugSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData) override {}

        void Update(SceneData* sceneData, float deltaTime) override
        {
            for (auto [entity, light, transform] : ECSManager->EntitiesWith<Light, Transform>())
            {
                if(light.Data.Type == LightType::Directional)
                {
                    Vector3 cameraUp, cameraRight;
                    for (auto [cameraEntity, camera, cameraTransform] : ECSManager->EntitiesWith<Camera, Transform>())
                    {
                        cameraUp = cameraTransform.GetUpVector();
                        cameraRight = cameraTransform.GetRightVector();
                    }
                    
                    static float lastMouseX = 0;
                    static float lastMouseY = 0;
                
                    auto [mouseX, mouseY] = Input::GetMousePos();

                    if (Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_RIGHT))
                    {
                        cameraRight.y = 0;
                        cameraUp.y = 0;
                    
                        float deltaX = (mouseX - lastMouseX) * deltaTime;
                        float deltaY = (mouseY - lastMouseY) * deltaTime;

                        Matrix4 rotationMatrix = rotate(Matrix4(1.0f), deltaX, cameraUp) * rotate(Matrix4(1.0f), deltaY, cameraRight);
                        LightTargetDirection = normalize(Vector3(rotationMatrix * Vector4(LightTargetDirection, 0.0f)));

                        transform.LookAt(transform.GetPosition() + LightTargetDirection);
                    }
                
                    lastMouseX = mouseX;
                    lastMouseY = mouseY;
                }
            }
        }
    };
}
