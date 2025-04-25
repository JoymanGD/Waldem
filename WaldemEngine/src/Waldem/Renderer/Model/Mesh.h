#pragma once
#include "Material.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    struct Vertex
    {
        Vector3 Position;
        Vector3 Normal;
        Vector3 Tangent;
        Vector3 Bitangent;
        Vector2 UV;
        uint MeshId;

        Vertex(Vector3 position, Vector3 normal, Vector3 tangent, Vector3 bitangent, Vector2 uv, uint meshId) : Position(position), Normal(normal), Tangent(tangent), Bitangent(bitangent), UV(uv), MeshId(meshId) {}
        Vertex() {}
    };
    
    class WALDEM_API CMesh : public Asset
    {
    public:
        CMesh() : Asset(AssetType::Mesh) {}
        CMesh(const CMesh& other) : Asset(other), VertexBuffer(other.VertexBuffer), IndexBuffer(other.IndexBuffer), CurrentMaterial(other.CurrentMaterial), Positions(other.Positions), BBox(other.BBox), ObjectMatrix(other.ObjectMatrix), Name(other.Name)
        {
            Type = AssetType::Mesh;
        }
        CMesh(WArray<Vertex> vertexData, WArray<uint> indexData, Material* material, AABB bBox, WString name = "", Matrix4 objectMatrix = glm::identity<Matrix4>());
        void SetMaterial(Material* material) { CurrentMaterial = material; }
        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
        Material* CurrentMaterial = nullptr;
        WArray<Vector3> Positions;
        WArray<Vertex> VertexData;
        WArray<uint> IndexData;
        AABB BBox;
        Matrix4 ObjectMatrix;
        WString Name;
    };
}