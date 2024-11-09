#pragma once
#include "Material.h"
#include "Transform.h"
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
        Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indices, uint32_t indicesAmount, const BufferLayout& layout, Material material);

        IndexBuffer* IB;
        VertexBuffer* VB;
        Material MeshMaterial;
    };
}
