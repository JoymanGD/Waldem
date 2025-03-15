#pragma once
#include "Material.h"
#include "Transform.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Buffer.h"

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
    
    class WALDEM_API CMesh
    {
    public:
        CMesh() = default;
        CMesh(const CMesh& other) : VertexBuffer(other.VertexBuffer), IndexBuffer(other.IndexBuffer), CurrentMaterial(other.CurrentMaterial), Positions(other.Positions), BBox(other.BBox), ObjectMatrix(other.ObjectMatrix), Name(other.Name) {}
        CMesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indexBufferData, uint32_t indexBufferDataSize, WArray<Vector3> positions, Material* material, AABB bBox, String name = "", Matrix4 objectMatrix = glm::identity<Matrix4>());
        void SetMaterial(Material* material) { CurrentMaterial = material; }

        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
        Material* CurrentMaterial = nullptr;
        WArray<Vector3> Positions;
        AABB BBox;
        Matrix4 ObjectMatrix;
        String Name;
    };
}