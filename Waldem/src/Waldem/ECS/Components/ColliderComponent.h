#pragma once

#include "Waldem/Renderer/Model/Mesh.h"
#include <execution>
#include <mutex>

namespace Waldem
{
    enum ColliderType
    {
        WD_COLLIDER_TYPE_NONE = 0,
        WD_COLLIDER_TYPE_SPHERE = 1,
        WD_COLLIDER_TYPE_BOX = 2,
        WD_COLLIDER_TYPE_CAPSULE = 3,
        WD_COLLIDER_TYPE_MESH = 4
    };
    
    // struct IColliderComponent
    // {
    //     virtual ColliderType GetType() { return WD_COLLIDER_TYPE_NONE; }
    //
    //     virtual Vector3 FindFurthestPoint(Vector3 dir) const { return Vector3(0.0f); }
    //
    //     bool IsColliding = false;
    // };

    struct SphereColliderData
    {
        float Radius = 1;
    };

    struct CapsuleColliderData
    {
        float Radius = 1;
        float Height = 1;
    };

    struct MeshColliderData
    {
        CMesh* Mesh = nullptr;
    };

    struct BoxColliderData
    {
        Vector3 Size = { 1, 1, 1 };
    };
    
    struct ColliderComponent
    {
        ColliderType Type = WD_COLLIDER_TYPE_BOX;

        ColliderComponent(ColliderType type, SphereColliderData data)
            : Type(type), SphereData(data) {}
        
        ColliderComponent(ColliderType type, CapsuleColliderData data)
            : Type(type), CapsuleData(data) {}
        
        ColliderComponent(ColliderType type, MeshColliderData data)
            : Type(type), MeshData(data) {}

        ColliderComponent(ColliderType type, BoxColliderData data)
            : Type(type), BoxData(data) {}

        Vector3 FindFurthestPoint(Vector3 dir, Transform& worldTransform) const
        {
            switch (Type)
            {
                case WD_COLLIDER_TYPE_SPHERE:
                {
                    return worldTransform.Position + normalize(dir) * SphereData.Radius;
                }
                case WD_COLLIDER_TYPE_CAPSULE:
                {
                    Vector3 top = worldTransform.Matrix * Vector4(0, CapsuleData.Height * 0.5f, 0, 1);
                    Vector3 bottom = worldTransform.Matrix * Vector4(0, -CapsuleData.Height * 0.5f, 0, 1);
                    Vector3 furthestEnd = (dot(top, dir) > dot(bottom, dir)) ? top : bottom;
                    return furthestEnd + normalize(dir) * CapsuleData.Radius;
                }
                case WD_COLLIDER_TYPE_BOX:
                {
                    Vector3 transformedSize = worldTransform.Matrix * Vector4(BoxData.Size, 1.0f);
                        
                    Vector3 localFurthest = Vector3(
                        (dir.x >= 0.0f) ? transformedSize.x : -transformedSize.x,
                        (dir.y >= 0.0f) ? transformedSize.y : -transformedSize.y,
                        (dir.z >= 0.0f) ? transformedSize.z : -transformedSize.z
                    );
                    return localFurthest;
                }
                case WD_COLLIDER_TYPE_MESH:
                {
                    Vector3 maxPoint = Vector3(0.0f);
                    float maxDistance = -FLT_MAX;
                    std::mutex mtx;
                    
                    std::for_each(std::execution::par, MeshData.Mesh->Positions.begin(), MeshData.Mesh->Positions.end(), [&](Vector3& vertex)
                    {
                        Vector3 transformedPosition = worldTransform.Matrix * Vector4(vertex, 1.0f);
                        float distance = dot(transformedPosition, dir);
                    
                        if (distance > maxDistance)
                        {
                            std::lock_guard lock(mtx);
                            maxDistance = distance;
                            maxPoint = transformedPosition;
                        }
                    });
                    
                    return maxPoint;
                }
            }
        }

        SphereColliderData SphereData;
        CapsuleColliderData CapsuleData;
        MeshColliderData MeshData;
        BoxColliderData BoxData;

        bool IsColliding = false;
    };
}
