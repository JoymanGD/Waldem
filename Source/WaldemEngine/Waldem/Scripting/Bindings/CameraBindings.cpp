#include "wdpch.h"
#include "CameraBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

namespace Waldem::Bindings
{
    namespace
    {
        float Camera_GetFieldOfView(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Camera>()) return 0.0f;
            return entity.get<Camera>().FieldOfView;
        }

        void Camera_SetFieldOfView(uint64_t entityId, float fov)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Camera>()) return;
            auto& cam = entity.get_mut<Camera>();
            cam.UpdateProjectionMatrix(fov, cam.AspectRatio, cam.NearPlane, cam.FarPlane);
            entity.modified<Camera>();
        }

        float Camera_GetNearPlane(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Camera>()) return 0.0f;
            return entity.get<Camera>().NearPlane;
        }

        void Camera_SetNearPlane(uint64_t entityId, float nearPlane)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Camera>()) return;
            auto& cam = entity.get_mut<Camera>();
            cam.UpdateProjectionMatrix(cam.FieldOfView, cam.AspectRatio, nearPlane, cam.FarPlane);
            entity.modified<Camera>();
        }

        float Camera_GetFarPlane(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Camera>()) return 0.0f;
            return entity.get<Camera>().FarPlane;
        }

        void Camera_SetFarPlane(uint64_t entityId, float farPlane)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Camera>()) return;
            auto& cam = entity.get_mut<Camera>();
            cam.UpdateProjectionMatrix(cam.FieldOfView, cam.AspectRatio, cam.NearPlane, farPlane);
            entity.modified<Camera>();
        }
        
        uint64_t Camera_GetMainEntity()
        {
            SViewport* gameViewport = ViewportManager::GetGameViewport();
            if(gameViewport == nullptr)
                return 0;

            ECS::Entity cameraEntity;
            if(!gameViewport->TryGetLinkedCamera(cameraEntity))
                return 0;

            if(!cameraEntity.is_alive() || !cameraEntity.has<Camera>())
                return 0;

            return cameraEntity.id();
        }
    }

    void RegisterCameraCalls(Mono* runtime)
    {
        BIND(runtime, Camera_GetFieldOfView);
        BIND(runtime, Camera_SetFieldOfView);
        BIND(runtime, Camera_GetNearPlane);
        BIND(runtime, Camera_SetNearPlane);
        BIND(runtime, Camera_GetFarPlane);
        BIND(runtime, Camera_SetFarPlane);
        BIND(runtime, Camera_GetMainEntity);
    }
}
