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
            FIELD(Vector3, BoxSize)
            FIELD(Vector3, BoxOffset)
            FIELD(float, SphereRadius)
            FIELD(float, CapsuleHeight)
            FIELD(ColliderType, Type)
        END_COMPONENT()
        
        Vector3 BoxSize = Vector3(1, 1, 1);
        Vector3 BoxOffset = Vector3(0, 0, 0);
        float SphereRadius = 1.0f;
        float CapsuleHeight = 1.0f;
        ColliderType Type = Box;
        MeshColliderData MeshData;
        bool IsColliding = false;

        Vector3 FindFurthestPoint(Vector3 dir, Transform& worldTransform) const
        {
            switch (Type)
            {
                case Sphere:
                {
                    return worldTransform.Position + normalize(dir) * SphereRadius;
                }
                case Capsule:
                {
                    Vector3 top = worldTransform.Matrix * Vector4(0, CapsuleHeight * 0.5f, 0, 1);
                    Vector3 bottom = worldTransform.Matrix * Vector4(0, -CapsuleHeight * 0.5f, 0, 1);
                    Vector3 furthestEnd = (dot(top, dir) > dot(bottom, dir)) ? top : bottom;
                    return furthestEnd + normalize(dir) * SphereRadius;
                }
                case Box:
                {
                    Vector3 half = BoxSize * 0.5f;

                    Vector3 localCorners[8] = {
                        { -half.x, -half.y, -half.z },
                        {  half.x, -half.y, -half.z },
                        {  half.x,  half.y, -half.z },
                        { -half.x,  half.y, -half.z },
                        { -half.x, -half.y,  half.z },
                        {  half.x, -half.y,  half.z },
                        {  half.x,  half.y,  half.z },
                        { -half.x,  half.y,  half.z },
                    };

                    Vector3 furthestPoint;
                    float maxDist = -FLT_MAX;

                    for (int i = 0; i < 8; ++i)
                    {
                        // Apply the local box offset before transforming
                        Vector3 localCorner = localCorners[i] + BoxOffset;
                        Vector3 worldCorner = worldTransform.Matrix * Vector4(localCorner, 1.0f);

                        float dist = dot(worldCorner, dir);
                        if (dist > maxDist)
                        {
                            maxDist = dist;
                            furthestPoint = worldCorner;
                        }
                    }

                    return furthestPoint;
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

        Matrix3 outer(const Vector3& a, const Vector3& b)
        {
            return Matrix3(
                a.x * b.x, a.x * b.y, a.x * b.z,
                a.y * b.x, a.y * b.y, a.y * b.z,
                a.z * b.x, a.z * b.y, a.z * b.z
            );
        }

        Matrix3 ComputeInertiaTensor(float mass)
        {
            switch (Type)
            {
            case Sphere:
                return (2.0f / 5.0f) * mass * SphereRadius * SphereRadius;
            case Box:
                {
                    float x2 = BoxSize.x * BoxSize.x;
                    float y2 = BoxSize.y * BoxSize.y;
                    float z2 = BoxSize.z * BoxSize.z;

                    Matrix3 I = (mass / 12.0f) * Matrix3(
                        y2 + z2, 0, 0,
                        0, x2 + z2, 0,
                        0, 0, x2 + y2
                    );

                    Vector3 r = BoxOffset;
                    float r2 = dot(r, r);
                    Matrix3 offsetTerm = mass * (r2 * Matrix3(1.0f) - outer(r, r));

                    return I + offsetTerm;
                }
            case Capsule:
                {
                    float radius2 = SphereRadius * SphereRadius;
                    float height2 = CapsuleHeight * CapsuleHeight;
                    float cylinderMass = mass * (CapsuleHeight / (CapsuleHeight + 2.0f * SphereRadius));
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
