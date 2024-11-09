#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    
    class WALDEM_API Camera
    {
    public:
        float MovementSpeed = 1.0f;
        float RotationSpeed = 1.0f;

        float Yaw = 0.0f;
        float Pitch = 0.0f;
        float Roll = 0.0f;
        
        Camera(float fov, float aspectRatio, float nearClip, float farClip, Vector3 position, float movementSpeed, float rotationSpeed);
        glm::mat4 GetViewProjectionMatrix();
        void Move(Vector3 delta);
        void Rotate(float yaw, float pitch, float roll);

    private:
        Transform WorldTransform;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewProjectionMatrix;
        
        const float ROTATION_COEF = 100.0f;
    };
}
