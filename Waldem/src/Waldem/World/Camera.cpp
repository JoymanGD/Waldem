#include "wdpch.h"
#include "Camera.h"
#include "glm/ext/matrix_clip_space.hpp"

namespace Waldem
{
    glm::mat4 ZFlipMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f));
    
    Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip, glm::vec3 position, float movementSpeed, float rotationSpeed)
    {
        WorldTransform = Transform(position);
        ProjectionMatrix = glm::perspective(fov * glm::pi<float>() / 180.0f, aspectRatio, nearClip, farClip);
        MovementSpeed = movementSpeed;
        RotationSpeed = rotationSpeed;
    }

    glm::mat4 Camera::GetViewProjectionMatrix()
    {
        ViewProjectionMatrix = ProjectionMatrix * ZFlipMatrix * WorldTransform.Inverse();
        return ViewProjectionMatrix;
    }

    void Camera::Move(glm::vec3 delta)
    {
        glm::vec3 forward = WorldTransform.GetForwardVector();
        glm::vec3 up = WorldTransform.GetUpVector();
        glm::vec3 right = WorldTransform.GetRightVector();
        WorldTransform.Translate(forward * -delta.z + right * delta.x + up * delta.y);
    }

    void Camera::Rotate(float yaw, float pitch, float roll)
    {
        // WorldTransform.Rotate(glm::angleAxis(glm::radians(deltaX) * RotationSpeed, glm::vec3(0, 1, 0)));
        // WorldTransform.Rotate(glm::angleAxis(glm::radians(deltaY) * RotationSpeed, glm::vec3(1, 0, 0)));

        Yaw += yaw * RotationSpeed;
        Pitch += pitch * RotationSpeed;
        Roll += roll * RotationSpeed;

        // Clamp the pitch angle to prevent it from going too far up or down
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);

        // Update the camera's rotation based on the new yaw and pitch angles
        WorldTransform.SetRotation(glm::quat(glm::vec3(Pitch, Yaw, Roll)));
    }
}
