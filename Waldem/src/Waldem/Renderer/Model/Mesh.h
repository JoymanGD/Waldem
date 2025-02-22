#pragma once
#include "Material.h"
#include "Transform.h"
#include "Waldem/Renderer/BoundingBox.h"
#include "Waldem/Renderer/Buffer.h"

namespace Waldem
{
    struct Vertex
    {
        Vector3 Position;
        Vector3 Normal;
        Vector3 Tangent;
        Vector2 UV;
        uint MeshId;

        Vertex(Vector3 position, Vector3 normal, Vector3 tangent, Vector2 uv, uint meshId) : Position(position), Normal(normal), Tangent(tangent), UV(uv), MeshId(meshId) {}
        Vertex() {}
    };
    
    class WALDEM_API Mesh
    {
    public:
        Mesh() = default;
        Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indexBufferData, uint32_t indexBufferDataSize, Material* material, BoundingBox bBox);
        void SetMaterial(Material* material) { CurrentMaterial = material; }

        Buffer* VertexBuffer;
        Buffer* IndexBuffer;
        Material* CurrentMaterial;
        BoundingBox BBox;
        Matrix4 ObjectMatrix;
    };
}
