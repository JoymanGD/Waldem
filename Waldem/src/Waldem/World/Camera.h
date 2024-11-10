#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    inline Matrix4 ZFlipMatrix = glm::scale(Matrix4(1.0f), Vector3(1.0f, 1.0f, -1.0f));
    
    class WALDEM_API Camera
    {
    public:
        Transform WorldTransform;
        
        float MovementSpeed = 1.0f;
        float RotationSpeed = 1.0f;

        float Yaw = 0.0f;
        float Pitch = 0.0f;
        float Roll = 0.0f;
        
        Camera(float fov, float aspectRatio, float nearClip, float farClip, Vector3 position, float movementSpeed, float rotationSpeed);
        glm::mat4 GetViewMatrix() { return ZFlipMatrix * WorldTransform.Inverse(); }
        glm::mat4 GetProjectionMatrix() { return ProjectionMatrix; }
        void Move(Vector3 delta);
        void Rotate(float yaw, float pitch, float roll);

    private:
        glm::mat4 ProjectionMatrix;
        
        const float ROTATION_COEF = 100.0f;
    };
}
