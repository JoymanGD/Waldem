#include "wdpch.h"
#include "Camera.h"
#include "glm/ext/matrix_clip_space.hpp"

namespace Waldem
{
    Matrix4 ZFlipMatrix = glm::scale(Matrix4(1.0f), Vector3(1.0f, 1.0f, -1.0f));
    
    Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip, Vector3 position, float movementSpeed, float rotationSpeed)
    {
        WorldTransform = Transform(position);
        ProjectionMatrix = glm::perspective(fov * glm::pi<float>() / 180.0f, aspectRatio, nearClip, farClip);
        MovementSpeed = movementSpeed;
        RotationSpeed = rotationSpeed / ROTATION_COEF;
    }

    Matrix4 Camera::GetViewProjectionMatrix()
    {
        ViewProjectionMatrix = ProjectionMatrix * ZFlipMatrix * WorldTransform.Inverse();
        return ViewProjectionMatrix;
    }

    void Camera::Move(Vector3 delta)
    {
        Vector3 forward = WorldTransform.GetForwardVector();
        Vector3 up = WorldTransform.GetUpVector();
        Vector3 right = WorldTransform.GetRightVector();
        WorldTransform.Translate(forward * -delta.z + right * delta.x + up * delta.y);
    }

    void Camera::Rotate(float yaw, float pitch, float roll)
    {
        // WorldTransform.Rotate(glm::angleAxis(glm::radians(deltaX) * RotationSpeed, Vector3(0, 1, 0)));
        // WorldTransform.Rotate(glm::angleAxis(glm::radians(deltaY) * RotationSpeed, Vector3(1, 0, 0)));

        Yaw += yaw * RotationSpeed;
        Pitch += pitch * RotationSpeed;
        Roll += roll * RotationSpeed;

        // Clamp the pitch angle to prevent it from going too far up or down
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);

        // Update the camera's rotation based on the new yaw and pitch angles
        WorldTransform.SetRotation(Quaternion(Vector3(Pitch, Yaw, Roll)));
    }
}
