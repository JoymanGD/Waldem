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
        Matrix4 CachedViewProjMatrix;
        Vector3 frustumCorners[8];
        
    public:
        DebugSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}

        void CacheFrustrumCorners()
        {
            for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, MainCamera, Transform>())
            {
                Vector3 ndcCorners[8] =
                {
                    {-1.0f, -1.0f, 0}, //near-top-left
                    {1.0f, -1.0f, 0}, //near-top-right
                    {-1.0f, 1.0f, 0}, //near-bottom-left
                    {1.0f, 1.0f, 0}, //near-bottom-right
                    {-1.0f, -1.0f, 1.0f}, //far-top-left
                    {1.0f, -1.0f, 1.0f}, //far-top-right
                    {-1.0f, 1.0f, 1.0f}, //far-bottom-left
                    {1.0f, 1.0f, 1.0f}  //far-bottom-right
                };

                // Transform NDC corners to world space
                CachedViewProjMatrix = camera.GetProjectionMatrix() * camera.GetViewMatrix();
                auto invViewProjMatrix = inverse(CachedViewProjMatrix);
                
                for (int i = 0; i < 8; ++i)
                {
                    Vector4 corner(ndcCorners[i].x, ndcCorners[i].y, ndcCorners[i].z, 1.0f);
                    Vector4 worldPos = invViewProjMatrix * corner; // Use your matrix transform function
                    worldPos /= worldPos.w; // Perform perspective divide
                    frustumCorners[i] = Vector3(worldPos.x, worldPos.y, worldPos.z);
                }
                break;
            }
        }

        void DrawFrustum(Vector4 color, Matrix4 viewProjMatrix)
        {
            WArray<Line> lines;

            //near plane
            lines.Add({ frustumCorners[0], frustumCorners[1], color }); //top
            lines.Add({ frustumCorners[1], frustumCorners[3], color }); //right
            lines.Add({ frustumCorners[3], frustumCorners[2], color }); //bottom
            lines.Add({ frustumCorners[2], frustumCorners[0], color }); //left

            //far plane
            lines.Add({ frustumCorners[4], frustumCorners[5], color }); //top
            lines.Add({ frustumCorners[5], frustumCorners[7], color }); //right
            lines.Add({ frustumCorners[7], frustumCorners[6], color }); //bottom
            lines.Add({ frustumCorners[6], frustumCorners[4], color }); //left

            //connect near plane to far plane
            lines.Add({ frustumCorners[0], frustumCorners[4], color }); //top-left
            lines.Add({ frustumCorners[1], frustumCorners[5], color }); //top-right
            lines.Add({ frustumCorners[2], frustumCorners[6], color }); //bottom-left
            lines.Add({ frustumCorners[3], frustumCorners[7], color }); //bottom-right

            for (auto& line : lines)
            {
                line.ToClipSpace(viewProjMatrix);
            }
            
            Renderer::DrawLines(lines);
        }
        
        void Initialize(SceneData* sceneData) override
        {
            CacheFrustrumCorners();
        }

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

            // Matrix4 viewProj;
            //
            // for (auto [entity, camera, mainCamera, cameraTransform] : ECSManager->EntitiesWith<Camera, MainCamera, Transform>())
            // {
            //     viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
            //     break;
            // }
            //
            // if(Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_MIDDLE))
            // {
            //     CacheFrustrumCorners();
            // }
            
            // DrawFrustum({ 0.0f, 1.0f, 0.0f, 1.0f }, viewProj);
        }
    };
}
