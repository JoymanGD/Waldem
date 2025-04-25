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

    void Camera::Serialize(WDataBuffer& outData)
    {
        outData << ProjectionMatrix;
        outData << ViewMatrix;
        outData << MovementSpeed;
        outData << RotationSpeed;
        outData << SpeedModificator;
        SpeedParams.Serialize(outData);
        Frustrum.Serialize(outData);
    }

    void Camera::Deserialize(WDataBuffer& inData)
    {
        inData >> ProjectionMatrix;
        inData >> ViewMatrix;
        inData >> MovementSpeed;
        inData >> RotationSpeed;
        inData >> SpeedModificator;
        SpeedParams.Deserialize(inData);
        Frustrum.Deserialize(inData);
    }
}
