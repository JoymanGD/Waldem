#pragma once

#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Utils/PhysicsUtils.h"
#include "Waldem/ECS/Component.h"
#include "Waldem/Serialization/Serializable.h"

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

    struct BoxColliderData : ISerializable
    {
        Vector3 Size = { 1, 1, 1 };
        
        void Serialize(WDataBuffer& outData) override
        { 
            outData << Size;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Size;
        }
    };

    struct SphereColliderData : ISerializable
    {
        float Radius = 1;
        
        void Serialize(WDataBuffer& outData) override
        { 
            outData << Radius;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Radius;
        }
    };

    struct CapsuleColliderData : ISerializable
    {
        float Radius = 1;
        float Height = 1;
        
        void Serialize(WDataBuffer& outData) override
        { 
            outData << Radius;
            outData << Height;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Radius;
            inData >> Height;
        }
    };

    struct MeshColliderData : ISerializable
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
        
        void Serialize(WDataBuffer& outData) override
        { 
            Vertices.Serialize(outData);
            Indices.Serialize(outData);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            Vertices.Deserialize(inData);
            Indices.Deserialize(inData);
        }
    };
    
    struct WALDEM_API ColliderComponent : IComponent<ColliderComponent>
    {
        ColliderType Type = WD_COLLIDER_TYPE_BOX;
        SphereColliderData SphereData;
        CapsuleColliderData CapsuleData;
        MeshColliderData MeshData;
        BoxColliderData BoxData;
        bool IsColliding = false;

        ColliderComponent() = default;
        
        ColliderComponent(ColliderType type, SphereColliderData data)
            : Type(type), SphereData(data) {}
        
        ColliderComponent(ColliderType type, CapsuleColliderData data)
            : Type(type), CapsuleData(data) {}
        
        ColliderComponent(ColliderType type, MeshColliderData data)
            : Type(type), MeshData(data) {}

        ColliderComponent(ColliderType type, BoxColliderData data)
            : Type(type), BoxData(data) {}

        Vector3 FindFurthestPoint(Vector3 dir, Transform* worldTransform) const
        {
            switch (Type)
            {
                case WD_COLLIDER_TYPE_SPHERE:
                {
                    return worldTransform->Position + normalize(dir) * SphereData.Radius;
                }
                case WD_COLLIDER_TYPE_CAPSULE:
                {
                    Vector3 top = worldTransform->Matrix * Vector4(0, CapsuleData.Height * 0.5f, 0, 1);
                    Vector3 bottom = worldTransform->Matrix * Vector4(0, -CapsuleData.Height * 0.5f, 0, 1);
                    Vector3 furthestEnd = (dot(top, dir) > dot(bottom, dir)) ? top : bottom;
                    return furthestEnd + normalize(dir) * CapsuleData.Radius;
                }
                case WD_COLLIDER_TYPE_BOX:
                {
                    Vector3 transformedSize = worldTransform->Matrix * Vector4(BoxData.Size, 1.0f);
                        
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
            case WD_COLLIDER_TYPE_NONE:
                break;
            case WD_COLLIDER_TYPE_SPHERE:
                return (2.0f / 5.0f) * mass * SphereData.Radius * SphereData.Radius;
            case WD_COLLIDER_TYPE_BOX:
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
            case WD_COLLIDER_TYPE_CAPSULE:
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
            case WD_COLLIDER_TYPE_MESH:
                {
                    return PhysicsUtils::ComputeMeshInertiaTensor(MeshData.Vertices, MeshData.Indices, mass);
                }
            }

            return Matrix3(1.0f);
        }

        void Serialize(WDataBuffer& outData) override
        {
            outData << (uint)Type;

            switch (Type)
            {
            case WD_COLLIDER_TYPE_NONE:
                break;
            case WD_COLLIDER_TYPE_SPHERE:
                {
                    SphereData.Serialize(outData);
                    break;
                }
            case WD_COLLIDER_TYPE_BOX:
                {
                    BoxData.Serialize(outData);
                    break;
                }
            case WD_COLLIDER_TYPE_CAPSULE:
                {
                    CapsuleData.Serialize(outData);
                    break;
                }
            case WD_COLLIDER_TYPE_MESH:
                {
                    MeshData.Serialize(outData);
                    break;
                }
            }
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Type;

            switch (Type)
            {
            case WD_COLLIDER_TYPE_NONE:
                break;
            case WD_COLLIDER_TYPE_SPHERE:
                {
                    SphereData.Deserialize(inData);
                    break;
                }
            case WD_COLLIDER_TYPE_BOX:
                {
                    BoxData.Deserialize(inData);
                    break;
                }
            case WD_COLLIDER_TYPE_CAPSULE:
                {
                    CapsuleData.Deserialize(inData);
                    break;
                }
            case WD_COLLIDER_TYPE_MESH:
                {
                    MeshData.Deserialize(inData);
                    break;
                }
            }
        }
    };
}
