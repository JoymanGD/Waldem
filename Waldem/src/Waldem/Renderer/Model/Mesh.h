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
        Vector2 UV;
        uint32_t MeshId;
    };
    
    class WALDEM_API Mesh
    {
    public:
        Mesh() = default;
        Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indexBufferData, uint32_t indexBufferDataSize, Material material, BoundingBox bBox);

        Buffer* VertexBuffer;
        Buffer* IndexBuffer;
        Material MeshMaterial;
        BoundingBox BBox;
        Matrix4 ObjectMatrix;
    };
}
