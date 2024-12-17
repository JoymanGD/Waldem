#include "wdpch.h"
#include "Camera.h"

namespace Waldem
{
    Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip, float movementSpeed, float rotationSpeed)
    {
        ProjectionMatrix = glm::perspective(fov * glm::pi<float>() / 180.0f, aspectRatio, nearClip, farClip);
        MovementSpeed = movementSpeed;
        RotationSpeed = rotationSpeed;

        ViewMatrix = glm::identity<Matrix4>();

        Frustrum.GetPlanes(ProjectionMatrix * ViewMatrix);
    }
}
