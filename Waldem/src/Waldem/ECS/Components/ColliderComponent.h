#pragma once

#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    enum ColliderType
    {
        WD_COLLIDER_TYPE_NONE = 0,
        WD_COLLIDER_TYPE_BOX = 1,
        WD_COLLIDER_TYPE_SPHERE = 2,
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
        float Radius;
    };

    struct CapsuleColliderData
    {
        float Radius;
        float Height;
    };

    struct MeshColliderData
    {
        CMesh Mesh;
    };

    struct BoxColliderData
    {
        Vector3 Size;
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
                    return Vector3(0.0f);
                case WD_COLLIDER_TYPE_CAPSULE:
                    return Vector3(0.0f);
                case WD_COLLIDER_TYPE_MESH:
                {
                    Vector3 maxPoint = Vector3(0.0f);
                    float maxDistance = -FLT_MAX;

                    for(Vector3 vertex : MeshData.Mesh.Positions)
                    {
                        Vector3 transformedPosition = worldTransform.Matrix * Vector4(vertex, 1.0f);
                        
                        float distance = dot(transformedPosition, dir);
                        if (distance > maxDistance)
                        {
                            maxDistance = distance;
                            maxPoint = transformedPosition;
                        }
                    }

                    return maxPoint;
                }
                case WD_COLLIDER_TYPE_BOX:
                    return Vector3(0.0f);
            }
        }

        SphereColliderData SphereData;
        CapsuleColliderData CapsuleData;
        MeshColliderData MeshData;
        BoxColliderData BoxData;

        bool IsColliding = false;
    };
}
