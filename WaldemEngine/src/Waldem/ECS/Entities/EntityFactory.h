#pragma once
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Light.h"

namespace Waldem
{
    class WALDEM_API EntityFactory
    {
    public:
        static ECS::Entity CreateCamera(float fov, float aspectRatio, float nearClip, float farClip)
        {
            auto entity = ECS::CreateSceneEntity("Camera");
            entity.set<Camera>({
                .FieldOfView = fov,
                .AspectRatio = aspectRatio,
                .NearPlane = nearClip,
                .FarPlane = farClip,
                .MovementSpeed = 30.0f,
                .RotationSpeed = 30.0f
            });

            return entity;
        }

        static ECS::Entity CreateDirectionalLight(Vector3 color, float intensity)
        {
            auto entity = ECS::CreateSceneEntity("DirectionalLight");
            entity.set<Light>({ color, intensity });
            return entity;
        }

        static ECS::Entity CreatePointLight(Vector3 color, float intensity, float radius)
        {
            auto entity = ECS::CreateSceneEntity("PointLight");
            entity.set<Light>({ color, intensity, radius });
            return entity;
        }

        static ECS::Entity CreateSpotLight(Vector3 color, float intensity, float radius, float outerCone, float softness)
        {
            auto entity = ECS::CreateSceneEntity("SpotLight");
            entity.set<Light>({ color, intensity, radius, outerCone, softness });
            return entity;
        }
    };
}
