#pragma once

#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Utils/PhysicsUtils.h"

namespace Waldem
{
    struct BVHNode
    {
        AABB Box;
        BVHNode* Left;
        BVHNode* Right;
        int ObjectIndex;
        WString DebugName;

        bool IsLeaf() const { return Left == nullptr && Right == nullptr; }
    };

    struct ReferenceFaceInfo
    {
        Vector3 Normal;
        Vector3 Center;
        Vector3 Vertices[4];
        int VertexCount = 0;
    };

    struct EPAResult
    {
        bool HasCollision;
        Vector3 Normal;
        float PenetrationDepth;
        Vector3 ContactCenterPosition;
        Vector3 ContactPositionA;
        Vector3 ContactPositionB; 
    };

    struct ContactPoint
    {
        Vector3 Position;
        Vector3 PositionA;
        Vector3 PositionB;
        Vector3 Normal;
        float Penetration;
        Matrix3 ContactToWorld;
        Matrix3 WorldToContact;

        void CalculateBasic()
        {
            Vector3 contactTangent[2];
            // Check whether the Z axis is nearer to the X or Y axis.
            if(abs(Normal.x) > abs(Normal.y))
            {
                // Scaling factor to ensure the results are normalized.
                const float s = 1.0f/sqrt(Normal.z * Normal.z + Normal.x * Normal.x);
                // The new X axis is at right angles to the world Y axis.
                contactTangent[0].x = Normal.z * s;
                contactTangent[0].y = 0;
                contactTangent[0].z = -Normal.x * s;
                // The new Y axis is at right angles to the new X and Z axes.
                contactTangent[1].x = Normal.y * contactTangent[0].x;
                contactTangent[1].y = Normal.z * contactTangent[0].x - Normal.x * contactTangent[0].z;
                contactTangent[1].z = -Normal.y * contactTangent[0].x;
            }
            else
            {
                // Scaling factor to ensure the results are normalized.
                const float s = (float)1.0/sqrt(Normal.z * Normal.z + Normal.y * Normal.y);
                // The new X axis is at right angles to the world X axis.
                contactTangent[0].x = 0;
                contactTangent[0].y = -Normal.z * s;
                contactTangent[0].z = Normal.y * s;
                // The new Y axis is at right angles to the new X and Z axes.
                contactTangent[1].x = Normal.y * contactTangent[0].z - Normal.z * contactTangent[0].y;
                contactTangent[1].y = -Normal.x * contactTangent[0].z;
                contactTangent[1].z = Normal.x * contactTangent[0].y;
            }
            
            // Make a matrix from the three vectors.
            ContactToWorld[0] = Normal;
            ContactToWorld[1] = contactTangent[0];
            ContactToWorld[2] = contactTangent[1];

            WorldToContact = transpose(ContactToWorld);
        }
    };

    struct ContactsManifold
    {
        WArray<ContactPoint> Points;
    };

    struct PersistentContact
    {
        ContactsManifold Manifold;
        int ColliderA;
        int ColliderB;
        bool WasUpdatedThisFrame = false;
        int FramesAlive = 0;
        Vector3 AccumulatedNormalImpulse = Vector3(0.0f); // Accumulated normal impulse for warm-starting
        Vector3 AccumulatedFrictionImpulse = Vector3(0.0f); // Accumulated friction impulse for warm-starting
        Vector3 SmoothedNormal = Vector3(0.0f); // Smoothed contact normal for stability
        WArray<float> NormalImpulses;
    };
    
    using CollisionPair = std::pair<int, int>;
    
    enum ColliderType
    {
        Sphere,
        Box,
        Capsule,
        Mesh
    };

    enum class CollisionEventType
    {
        Enter,
        Stay,
        Exit
    };
    
    COMPONENT()
    struct WALDEM_API ColliderComponent
    {        
        FIELD()
        Vector3 BoxSize = Vector3(1, 1, 1);
        FIELD()
        Vector3 BoxOffset = Vector3(0, 0, 0);
        FIELD()
        float SphereRadius = 1.0f;
        FIELD()
        float CapsuleHeight = 1.0f;
        FIELD()
        ColliderType Type = Box;
        FIELD()
        bool IsTrigger = false;
        
        WArray<Vector3> Vertices;
        WArray<uint> Indices;

        std::function<void(ECS::Entity, const ContactsManifold&)> OnCollisionEnter;
        std::function<void(ECS::Entity, const ContactsManifold&)> OnCollisionStay;
        std::function<void(ECS::Entity, const ContactsManifold&)> OnCollisionExit;
        
        ColliderComponent() {}

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

                    for (auto vertex : Vertices)
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

        Matrix3 Outer(const Vector3& a, const Vector3& b)
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
                    Vector3 half = BoxSize * 0.5f;
                    float x2 = half.x * half.x;
                    float y2 = half.y * half.y;
                    float z2 = half.z * half.z;

                    Matrix3 I = (mass / 12.0f) * Matrix3(
                        y2 + z2, 0, 0,
                        0, x2 + z2, 0,
                        0, 0, x2 + y2
                    );

                    Vector3 r = BoxOffset;
                    float r2 = dot(r, r);
                    Matrix3 offsetTerm = mass * (r2 * Matrix3(1.0f) - Outer(r, r));

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
                    return PhysicsUtils::ComputeMeshInertiaTensor(Vertices, Indices, mass);
                }
            }

            return Matrix3(1.0f);
        }

        ReferenceFaceInfo GetReferenceFace(const Transform& tr, const Vector3& worldNormal)
        {
            ReferenceFaceInfo ref;

            switch (Type)
            {
                case Box:
                {
                    Matrix3 R = Matrix3(tr.RotationQuat);
                    Vector3 nLocal = normalize(transpose(R) * worldNormal);

                    Vector3 localNormals[6] = {
                        {  1, 0, 0 }, { -1, 0, 0 },
                        {  0, 1, 0 }, {  0,-1, 0 },
                        {  0, 0, 1 }, {  0, 0,-1 }
                    };

                    int bestFace = 0;
                    float bestDot = -FLT_MAX;
                    for (int i = 0; i < 6; ++i)
                    {
                        float d = dot(localNormals[i], nLocal);
                        if (d > bestDot) { bestDot = d; bestFace = i; }
                    }

                    Vector3 half = BoxSize * 0.5f;
                    Vector3 localVerts[8] = {
                        {-half.x,-half.y,-half.z},{ half.x,-half.y,-half.z},
                        { half.x, half.y,-half.z},{-half.x, half.y,-half.z},
                        {-half.x,-half.y, half.z},{ half.x,-half.y, half.z},
                        { half.x, half.y, half.z},{-half.x, half.y, half.z}
                    };

                    static const int faceIndices[6][4] = {
                        {1,5,6,2}, {4,0,3,7}, {3,2,6,7},
                        {4,5,1,0}, {5,4,7,6}, {0,1,2,3}
                    };

                    Vector3 localNormal = localNormals[bestFace];
                    ref.Normal = normalize(R * localNormal);

                    for (int i = 0; i < 4; ++i)
                        ref.Vertices[i] = tr.Position + R * (localVerts[faceIndices[bestFace][i]] + BoxOffset);
                    ref.VertexCount = 4;

                    ref.Center = (ref.Vertices[0] + ref.Vertices[1] + ref.Vertices[2] + ref.Vertices[3]) * 0.25f;
                    break;
                }

                case Sphere:
                {
                    ref.Normal = worldNormal;
                    ref.Center = tr.Position + worldNormal * SphereRadius;
                    ref.VertexCount = 0;
                    break;
                }

                case Capsule:
                {
                    Matrix3 R = Matrix3(tr.RotationQuat);
                    Vector3 axis = R * Vector3(0, 1, 0);
                    float dotAxis = dot(axis, worldNormal);

                    if (fabs(dotAxis) > 0.707f) // ~cos(45°)
                    {
                        ref.Normal = worldNormal;
                        Vector3 capCenter = tr.Position + axis * glm::sign(dotAxis) * (CapsuleHeight * 0.5f);
                        ref.Center = capCenter + worldNormal * SphereRadius;
                        ref.VertexCount = 0;
                    }
                    else
                    {
                        Vector3 radial = normalize(worldNormal - axis * dot(worldNormal, axis));
                        ref.Normal = radial;
                        ref.Center = tr.Position + radial * SphereRadius;
                        ref.VertexCount = 0;
                    }
                    break;
                }

                case Mesh:
                {
                    float bestDot = -FLT_MAX;
                    int bestTri = -1;
                    for (size_t i = 0; i < Indices.Num(); i += 3)
                    {
                        Vector3 a = Vertices[Indices[i]];
                        Vector3 b = Vertices[Indices[i+1]];
                        Vector3 c = Vertices[Indices[i+2]];
                        Vector3 n = normalize(cross(b - a, c - a));
                        float d = dot(n, worldNormal);
                        if (d > bestDot) { bestDot = d; bestTri = (int)i; }
                    }

                    if (bestTri >= 0)
                    {
                        Matrix4 M = tr.Matrix;
                        Vector3 a = M * Vector4(Vertices[Indices[bestTri + 0]], 1.0f);
                        Vector3 b = M * Vector4(Vertices[Indices[bestTri + 1]], 1.0f);
                        Vector3 c = M * Vector4(Vertices[Indices[bestTri + 2]], 1.0f);

                        ref.Vertices[0] = a;
                        ref.Vertices[1] = b;
                        ref.Vertices[2] = c;
                        ref.VertexCount = 3;
                        ref.Normal = normalize(cross(b - a, c - a));
                        ref.Center = (a + b + c) / 3.0f;
                    }
                    break;
                }
            }

            return ref;
        }
    };
}
#include "ColliderComponent.generated.h"
