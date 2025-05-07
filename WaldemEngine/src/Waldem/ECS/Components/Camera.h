#pragma once
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Component.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    struct FrustumPlane : ISerializable
    {
        Vector3 Normal;
        float Distance;

        bool IsPointInFront(const Vector3& point) const
        {
            return dot(Normal, point) + Distance >= 0.0f;
        }

        void Serialize(WDataBuffer& outData) override
        {
            outData << Normal;
            outData << Distance;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Normal;
            inData >> Distance;
        }
    };

    struct Frustrum : ISerializable
    {
    private:
        WArray<FrustumPlane> Planes;

    public:
        WArray<FrustumPlane> GetPlanes(Matrix4 viewProjMatrix)
        {
            Planes.Clear();
            Planes.Resize(6);
            
            //left plane
            Planes[0].Normal.x = viewProjMatrix[0][3] + viewProjMatrix[0][0];
            Planes[0].Normal.y = viewProjMatrix[1][3] + viewProjMatrix[1][0];
            Planes[0].Normal.z = viewProjMatrix[2][3] + viewProjMatrix[2][0];
            Planes[0].Distance = viewProjMatrix[3][3] + viewProjMatrix[3][0];

            //right plane
            Planes[1].Normal.x = viewProjMatrix[0][3] - viewProjMatrix[0][0];
            Planes[1].Normal.y = viewProjMatrix[1][3] - viewProjMatrix[1][0];
            Planes[1].Normal.z = viewProjMatrix[2][3] - viewProjMatrix[2][0];
            Planes[1].Distance = viewProjMatrix[3][3] - viewProjMatrix[3][0];

            //bottom plane
            Planes[2].Normal.x = viewProjMatrix[0][3] + viewProjMatrix[0][1];
            Planes[2].Normal.y = viewProjMatrix[1][3] + viewProjMatrix[1][1];
            Planes[2].Normal.z = viewProjMatrix[2][3] + viewProjMatrix[2][1];
            Planes[2].Distance = viewProjMatrix[3][3] + viewProjMatrix[3][1];

            //top plane
            Planes[3].Normal.x = viewProjMatrix[0][3] - viewProjMatrix[0][1];
            Planes[3].Normal.y = viewProjMatrix[1][3] - viewProjMatrix[1][1];
            Planes[3].Normal.z = viewProjMatrix[2][3] - viewProjMatrix[2][1];
            Planes[3].Distance = viewProjMatrix[3][3] - viewProjMatrix[3][1];

            //near plane
            Planes[4].Normal.x = viewProjMatrix[0][3] + viewProjMatrix[0][2];
            Planes[4].Normal.y = viewProjMatrix[1][3] + viewProjMatrix[1][2];
            Planes[4].Normal.z = viewProjMatrix[2][3] + viewProjMatrix[2][2];
            Planes[4].Distance = viewProjMatrix[3][3] + viewProjMatrix[3][2];

            //far plane
            Planes[5].Normal.x = viewProjMatrix[0][3] - viewProjMatrix[0][2];
            Planes[5].Normal.y = viewProjMatrix[1][3] - viewProjMatrix[1][2];
            Planes[5].Normal.z = viewProjMatrix[2][3] - viewProjMatrix[2][2];
            Planes[5].Distance = viewProjMatrix[3][3] - viewProjMatrix[3][2];

            //normalize the Planes
            for (auto& plane : Planes)
            {
                float length = glm::length(plane.Normal);
                plane.Normal /= length;
                plane.Distance /= length;
            }

            return Planes;
        }

        void Serialize(WDataBuffer& outData) override
        {
            Planes.Serialize(outData);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            Planes.Deserialize(inData);
        }
    };

    struct WALDEM_API CameraSpeedParams : ISerializable
    {
        float ModificationStep = 0.1f;
        float MinSpeedModificator = 0.01f;
        float MaxSpeedModificator = 10.0f;

        void Serialize(WDataBuffer& outData) override
        {
            outData << ModificationStep;
            outData << MinSpeedModificator;
            outData << MaxSpeedModificator;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> ModificationStep;
            inData >> MinSpeedModificator;
            inData >> MaxSpeedModificator;
        }
    };

    struct WALDEM_API Camera : IComponent<Camera>
    {
        Camera() = default;
        
        Camera(float fov, float aspectRatio, float nearClip, float farClip, float movementSpeed, float rotationSpeed);

        void UpdateProjectionMatrix(float fov, float aspectRatio, float nearClip, float farClip);
        void SetViewMatrix(Transform* transform) { ViewMatrix = transform->Inverse(); }
        void SetViewMatrix(Matrix4 matrix) { ViewMatrix = matrix; }
        WArray<FrustumPlane> ExtractFrustumPlanes() { return Frustrum.GetPlanes(ProjectionMatrix * ViewMatrix); }
        
        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        float FieldOfView = 60.f;
        float AspectRatio = 1.f;
        float NearPlane = 0.001f;
        float FarPlane = 1000.0f;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewMatrix;
        float MovementSpeed = 1.0f;
        float RotationSpeed = 1.0f;
        float SpeedModificator = 1.0f;
        CameraSpeedParams SpeedParams;
        Frustrum Frustrum;
    };
}
