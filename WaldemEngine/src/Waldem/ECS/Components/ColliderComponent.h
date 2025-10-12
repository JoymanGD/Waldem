#pragma once

#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Utils/PhysicsUtils.h"

namespace Waldem
{
    enum ColliderType
    {
        Sphere,
        Box,
        Capsule,
        Mesh
    };

    struct BoxColliderData
    {
        Vector3 Size = { 1, 1, 1 };
    };

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
        WArray<Vector3> Vertices;
        WArray<uint> Indices;
        
        MeshColliderData() = default;
        
        MeshColliderData(CMesh* mesh)
        {
            Vertices = mesh->Positions;
            Indices = mesh->IndexData;
        }
        
        MeshColliderData(WArray<Vector3> vertices, WArray<uint> indices)
        {
            Vertices = vertices;
            Indices = indices;
        }
    };
    
    struct WALDEM_API ColliderComponent
    {
        COMPONENT(ColliderComponent)
            FIELD(ColliderType, Type)
        END_COMPONENT()
        
        ColliderType Type = Box;
        SphereColliderData SphereData;
        CapsuleColliderData CapsuleData;
        MeshColliderData MeshData;
        BoxColliderData BoxData;
        bool IsColliding = false;

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
                case Sphere:
                {
                    return worldTransform.Position + normalize(dir) * SphereData.Radius;
                }
                case Capsule:
                {
                    Vector3 top = worldTransform.Matrix * Vector4(0, CapsuleData.Height * 0.5f, 0, 1);
                    Vector3 bottom = worldTransform.Matrix * Vector4(0, -CapsuleData.Height * 0.5f, 0, 1);
                    Vector3 furthestEnd = (dot(top, dir) > dot(bottom, dir)) ? top : bottom;
                    return furthestEnd + normalize(dir) * CapsuleData.Radius;
                }
                case Box:
                {
                    Vector3 transformedSize = worldTransform.Matrix * Vector4(BoxData.Size, 1.0f);
                        
                    Vector3 localFurthest = Vector3(
                        (dir.x >= 0.0f) ? transformedSize.x : -transformedSize.x,
                        (dir.y >= 0.0f) ? transformedSize.y : -transformedSize.y,
                        (dir.z >= 0.0f) ? transformedSize.z : -transformedSize.z
                    );
                    return localFurthest;
                }
                case Mesh:
                {
                    Vector3 maxPoint = Vector3(0.0f);
                    float maxDistance = -FLT_MAX;

                    for (auto vertex : MeshData.Vertices)
                    {
                        float distance = dot(vertex, dir);
                    
                        if (distance > maxDistance)
                        {
                            maxDistance = distance;
                            maxPoint = vertex;
                        }
                    }
                    
                    return maxPoint;
                }
                default: return Vector3(0.0f);
            }
        }

        Matrix3 ComputeInertiaTensor(float mass)
        {
            switch (Type)
            {
            case Sphere:
                return (2.0f / 5.0f) * mass * SphereData.Radius * SphereData.Radius;
            case Box:
                {
                    float x2 = BoxData.Size.x * BoxData.Size.x;
                    float y2 = BoxData.Size.y * BoxData.Size.y;
                    float z2 = BoxData.Size.z * BoxData.Size.z;
                    return mass / 12.0f * Matrix3(
                        y2 + z2, 0, 0,
                        0, x2 + z2, 0,
                        0, 0, x2 + y2
                    );
                }
            case Capsule:
                {
                    float radius2 = CapsuleData.Radius * CapsuleData.Radius;
                    float height2 = CapsuleData.Height * CapsuleData.Height;
                    float cylinderMass = mass * (CapsuleData.Height / (CapsuleData.Height + 2.0f * CapsuleData.Radius));
                    float sphereMass = mass - cylinderMass;
                    float Ixx = (1.0f / 12.0f) * cylinderMass * (3.0f * radius2 + height2) + (2.0f / 5.0f) * sphereMass * radius2;
                    float Iyy = Ixx;
                    float Izz = (1.0f / 2.0f) * cylinderMass * radius2 + (2.0f / 5.0f) * sphereMass * radius2;
                    return Matrix3(
                        Ixx, 0, 0,
                        0, Iyy, 0,
                        0, 0, Izz
                    );
                }
            case Mesh:
                {
                    return PhysicsUtils::ComputeMeshInertiaTensor(MeshData.Vertices, MeshData.Indices, mass);
                }
            }

            return Matrix3(1.0f);
        }
    };
}
