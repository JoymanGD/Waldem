#pragma once
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    struct WALDEM_API Camera
    {
        Camera(float fov, float aspectRatio, float nearClip, float farClip, float movementSpeed, float rotationSpeed);

        void SetViewMatrix(Transform* transform) { ViewMatrix = transform->Inverse(); }
        Matrix4 GetViewMatrix() { return ViewMatrix; }
        Matrix4 GetProjectionMatrix() { return ProjectionMatrix; }

        float MovementSpeed = 1.0f;
        float RotationSpeed = 1.0f;
        
    private:
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewMatrix;
        
    };
}
