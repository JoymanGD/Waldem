#include "wdpch.h"
#include "Camera.h"

namespace Waldem
{
    Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip, float movementSpeed, float rotationSpeed)
    {
        FieldOfView = fov;
        AspectRatio = aspectRatio;
        NearPlane = nearClip;
        FarPlane = farClip;
        ProjectionMatrix = glm::perspective(fov * glm::pi<float>() / 180.0f, aspectRatio, nearClip, farClip);
        MovementSpeed = movementSpeed;
        RotationSpeed = rotationSpeed;

        ViewMatrix = glm::identity<Matrix4>();

        Frustrum.GetPlanes(ProjectionMatrix * ViewMatrix);
    }

    void Camera::UpdateProjectionMatrix(float fov, float aspectRatio, float nearClip, float farClip)
    {
        FieldOfView = fov;
        AspectRatio = aspectRatio;
        NearPlane = nearClip;
        FarPlane = farClip;
        ProjectionMatrix = glm::perspective(fov * glm::pi<float>() / 180.0f, aspectRatio, nearClip, farClip);

        ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    }
}
